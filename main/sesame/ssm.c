#include "os/os.h"
#include "ssm.h"
#include "blecent.h"
#include "c_ccm.h"
#include "ssm_cmd.h"

static const char * TAG = "ssm.c";

static uint8_t additional_data[] = { 0x00 };

struct ssm_env_tag * p_ssms_env[SSM_MAX_NUM];

static void *ssms_env_mem;
static struct os_mempool ssms_env_pools;

void
print_addr_ssm(const void *addr, const char *name)
{
    const uint8_t *u8p;
    u8p = addr;
    ESP_LOGI(TAG, "%s = %02x:%02x:%02x:%02x:%02x:%02x",
                name, u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}


static void ssm_initial_handle(sesame * ssm, uint8_t cmd_it_code) {
    ssm->cipher.encrypt.nouse = 0; // reset cipher
    ssm->cipher.decrypt.nouse = 0;
    memcpy(ssm->cipher.encrypt.random_code, ssm->b_buf, 4);
    memcpy(ssm->cipher.decrypt.random_code, ssm->b_buf, 4);
    ssm->cipher.encrypt.count = 0;
    ssm->cipher.decrypt.count = 0;

    //if (p_ssms_env->ssm.device_secret[0] == 0) {
    if (ssm->device_secret[0] == 0) {
        ESP_LOGI(TAG, "[ssm][no device_secret]");
        send_reg_cmd_to_ssm(ssm);
        return;
    }
    send_login_cmd_to_ssm(ssm);
}

static void ssm_parse_publish(sesame * ssm, uint8_t cmd_it_code) {
    switch (cmd_it_code) {
    case SSM_ITEM_CODE_INITIAL: // get 4 bytes random_code
        ssm_initial_handle(ssm, cmd_it_code);
        break;
    case SSM_ITEM_CODE_MECH_STATUS:
        memcpy((void *) &(ssm->mech_status), ssm->b_buf, 7);
        device_status_t lockStatus = ssm->mech_status.is_lock_range ? SSM_LOCKED : (ssm->mech_status.is_unlock_range ? SSM_UNLOCKED : SSM_MOVED);
        if (ssm->device_status != lockStatus) {
            ssm->device_status = lockStatus;
            struct ssm_env_tag * p_ssmsEnv = find_ssms_env_by_conn_id(ssm->conn_id);
            if(p_ssmsEnv != NULL){
                p_ssmsEnv->ssm_cb__(ssm); // callback: ssm_action_handle
                //p_ssms_env->ssm_cb__(ssm); // callback: ssm_action_handle
            }
        }
        break;
    default:
        break;
    }
}

static void ssm_parse_response(sesame * ssm, uint8_t cmd_it_code) {
    ssm->c_offset = ssm->c_offset - 1;
    memcpy(ssm->b_buf, ssm->b_buf + 1, ssm->c_offset);
    switch (cmd_it_code) {
    case SSM_ITEM_CODE_REGISTRATION:
        handle_reg_data_from_ssm(ssm);
        break;
    case SSM_ITEM_CODE_LOGIN:
        ESP_LOGI(TAG, "[%d][ssm][login][ok]", ssm->conn_id);
        ssm->device_status = SSM_LOGGIN;
    
        struct ssm_env_tag * p_ssmsEnv = find_ssms_env_by_conn_id(ssm->conn_id);
        if(p_ssmsEnv != NULL){
            p_ssmsEnv->ssm_cb__(ssm); // callback: ssm_action_handle
        }
//        p_ssms_env->ssm_cb__(ssm); // callback: ssm_action_handle
        break;
    case SSM_ITEM_CODE_HISTORY:
        ESP_LOGI(TAG, "[%d][ssm][hisdataLength: %d]", ssm->conn_id, ssm->c_offset);
        if (ssm->c_offset == 0) { //循環讀取 避免沒取完歷史
            return;
        }
        send_read_history_cmd_to_ssm(ssm);
        break;
    default:
        break;
    }
}

void ssm_ble_receiver(sesame * ssm, const uint8_t * p_data, uint16_t len) {
    if (p_data[0] & 1u) {
        ssm->c_offset = 0;
    }
    memcpy(&ssm->b_buf[ssm->c_offset], p_data + 1, len - 1);
    ssm->c_offset += len - 1;
    if (p_data[0] >> 1u == SSM_SEG_PARSING_TYPE_APPEND_ONLY) {
        return;
    }
    if (p_data[0] >> 1u == SSM_SEG_PARSING_TYPE_CIPHERTEXT) {
        ssm->c_offset = ssm->c_offset - CCM_TAG_LENGTH;
        aes_ccm_auth_decrypt(ssm->cipher.token, (const unsigned char *) &ssm->cipher.decrypt, 13, additional_data, 1, ssm->b_buf, ssm->c_offset, ssm->b_buf, ssm->b_buf + ssm->c_offset, CCM_TAG_LENGTH);
        ssm->cipher.decrypt.count++;
    }

    uint8_t cmd_op_code = ssm->b_buf[0];
    uint8_t cmd_it_code = ssm->b_buf[1];
    ssm->c_offset = ssm->c_offset - 2;
    memcpy(ssm->b_buf, ssm->b_buf + 2, ssm->c_offset);
    ESP_LOGI(TAG, "[ssm][say][%d][%s][%s]", ssm->conn_id, SSM_OP_CODE_STR(cmd_op_code), SSM_ITEM_CODE_STR(cmd_it_code));
    if (cmd_op_code == SSM_OP_CODE_PUBLISH) {
        ssm_parse_publish(ssm, cmd_it_code);
    } else if (cmd_op_code == SSM_OP_CODE_RESPONSE) {
        ssm_parse_response(ssm, cmd_it_code);
    }
    ssm->c_offset = 0;
}

void talk_to_ssm(sesame * ssm, uint8_t parsing_type) {
    ESP_LOGI(TAG, "[esp32][say][%d][%s]", ssm->conn_id, SSM_ITEM_CODE_STR(ssm->b_buf[0]));
    if (parsing_type == SSM_SEG_PARSING_TYPE_CIPHERTEXT) {
        aes_ccm_encrypt_and_tag(ssm->cipher.token, (const unsigned char *) &ssm->cipher.encrypt, 13, additional_data, 1, ssm->b_buf, ssm->c_offset, ssm->b_buf, ssm->b_buf + ssm->c_offset, CCM_TAG_LENGTH);
        ssm->cipher.encrypt.count++;
        ssm->c_offset = ssm->c_offset + CCM_TAG_LENGTH;
    }

    uint8_t * data = ssm->b_buf;
    uint16_t remain = ssm->c_offset;
    uint16_t len = remain;
    uint8_t tmp_v[20] = { 0 };
    uint16_t len_l;

    while (remain) {
        if (remain <= 19) {
            tmp_v[0] = parsing_type << 1u;
            len_l = 1 + remain;
        } else {
            tmp_v[0] = 0;
            len_l = 20;
        }
        if (remain == len) {
            tmp_v[0] |= 1u;
        }
        memcpy(&tmp_v[1], data, len_l - 1);
        esp_ble_gatt_write(ssm, tmp_v, len_l);
        remain -= (len_l - 1);
        data += (len_l - 1);
    }
}

void ssm_action_handle(sesame * ssm) {
    ESP_LOGI(TAG, "[ssm_action_handle][ssm status: %s]", SSM_STATUS_STR(ssm->device_status));
    if (ssm->device_status == SSM_UNLOCKED) {
        ssm_lock(NULL, 0,0);
    }
}

struct ssm_env_tag * find_free_ssms_env(){
    struct ssm_env_tag * free_ssms_env = NULL;
    
    for(int i=0;i<SSM_MAX_NUM;i++){
        if(SSM_NOUSE == p_ssms_env[i]->ssm.device_status){
            free_ssms_env = p_ssms_env[i];
            return free_ssms_env;
        }
    }
    return NULL;
}

struct ssm_env_tag * find_discnt_ssms_env_or_free(ble_addr_t * addr){
    struct ssm_env_tag * ssms_env = NULL;
    int i;
    
    for(i=0;i<SSM_MAX_NUM;i++){
        ESP_LOGI(TAG, "[find_discnt_ssms_env_or_free] ssms_env dev status=%d",p_ssms_env[i]->ssm.device_status );
        //print_addr_ssm((const void *)addr, "peer addr");
        //print_addr_ssm((const void *)p_ssms_env[i]->ssm.addr, "ssm_env->addr");
        if((SSM_DISCONNECTED == p_ssms_env[i]->ssm.device_status) && 
            (0 == memcmp(addr->val, p_ssms_env[i]->ssm.addr, sizeof(addr->val)))){
            ssms_env = p_ssms_env[i];
            ESP_LOGI(TAG, "[find_discnt_ssms_env_or_free] same addr ssms_env 0x%x",(unsigned int)ssms_env);
            return ssms_env;            
        }
    }
    
    for(i=0;i<SSM_MAX_NUM;i++){
        if(SSM_NOUSE == p_ssms_env[i]->ssm.device_status){
            ssms_env = p_ssms_env[i];
            ESP_LOGI(TAG, "[find_discnt_ssms_env_or_free] Nouse ssms_env 0x%x",(unsigned int)ssms_env);
            return ssms_env; 
        }
    }
    return NULL;
}


struct ssm_env_tag * find_ssms_env_by_conn_id(uint16_t conn_id){
        
    for(int i=0;i<SSM_MAX_NUM;i++){
        if(conn_id == p_ssms_env[i]->ssm.conn_id){
            return p_ssms_env[i];
        }
    }
    return NULL;
}

void ssm_mem_deinit(void) {
    free(ssms_env_mem);
    ssms_env_mem = NULL;

    for(int i=0;i<SSM_MAX_NUM;i++){
        p_ssms_env[i] = NULL;
    }
}

void ssm_init(int max_peers, ssm_action ssm_action_cb) {
    int rc;
    int buflen= OS_MEMPOOL_BYTES(max_peers, sizeof (struct ssm_env_tag));

    ssms_env_mem = malloc(buflen);
    if (ssms_env_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }
    ESP_LOGI(TAG, "[ssm_init] buf len=%d, ssms_env_mem = 0x%x", buflen, (unsigned int)ssms_env_mem);

    rc = os_mempool_init(&ssms_env_pools, max_peers,
                         sizeof (struct ssm_env_tag), ssms_env_mem,
                         "ssm_env_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }
    ESP_LOGI(TAG, "[ssm_init] ssm_env_pool inited");

    // Init p_ssms_env
    for(int i=0;i<max_peers;i++){
        //p_ssms_env[i] = (struct ssm_env_tag *) os_memblock_get(&ssms_env_pools); 
        p_ssms_env[i] = (struct ssm_env_tag *) calloc(1, sizeof(struct ssm_env_tag)); 
        
        if (p_ssms_env[i] == NULL) {
            ESP_LOGE(TAG, "[ssm_init][FAIL]");
        }
        ESP_LOGI(TAG, "[ssm_init] init %d ssms_env = 0x%x", i, (unsigned int)p_ssms_env[i]);

        p_ssms_env[i]->ssm_cb__ = ssm_action_cb; // callback: ssm_action_handle
        p_ssms_env[i]->ssm.conn_id = 0xFF;       // 0xFF: not connected
        p_ssms_env[i]->ssm.device_status = SSM_NOUSE;
        memset(p_ssms_env[i]->ssm.addr,0,6);
        memset(p_ssms_env[i]->ssm.device_uuid,0,16);
        memset(p_ssms_env[i]->ssm.public_key,0,64);
        memset(p_ssms_env[i]->ssm.device_secret,0,16);
    }
    ESP_LOGI(TAG, "[ssm_init][SUCCESS init %d devices]",max_peers);
    return;
    
err:
    ESP_LOGI(TAG, "ERR: [ssm_init][failed init %d devices]",max_peers);
    ssm_mem_deinit();
    return;
}
