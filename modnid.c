//
// Created by zhangdj on 2021/5/24.
//

#include "../../lib/lv_bindings/lvgl/lvgl.h"
//#include "../include/common.h"
#include "lib/lv_bindings/driver/include/common.h"
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include <winsock2.h>


static int sock ;
static struct sockaddr_in sock_addr ;
static pthread_t nid_thread ;

static lv_indev_data_t indata ;

bool nid_input_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    data->point.x = indata.point.x;
    data->point.y = indata.point.y;
    data->state = indata.state;
    return false;
}


void * thread_nid_server(void * args)
{
    while (true){
        struct sockaddr_in clnt_addr;
        int  clnt_addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

        char * buff = (char *) malloc(1024) ;

        memset(buff,0,1024) ;
        recv(clnt_sock,buff,1024,0) ;

        char * token ;

        token = strtok(buff,"#");
        indata.point.x = atoi(token) ;
        token = strtok(NULL,"#");
        indata.point.y = atoi(token) ;
        token = strtok(NULL,"#");
        if(strcmp((const char *)token,"PR") == 0){
            indata.state = LV_INDEV_STATE_PR ;
        } else{
            indata.state = LV_INDEV_STATE_REL ;
        }

        free(buff) ;

        close(clnt_sock);
    }
}


STATIC mp_obj_t nid_init(mp_obj_t addr_in,mp_obj_t port_in)
{
    const char * addr = mp_obj_str_get_str(addr_in) ;
    uint32_t port = mp_obj_get_int(port_in) ;

    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(1,1),&wsadata)==SOCKET_ERROR)
    {
        printf("WSAStartup() fail\n");
    }

    indata.point.x = 0 ;
    indata.point.y = 0 ;
    indata.state = LV_INDEV_STATE_REL ;

    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP) ;

    sock_addr.sin_family = AF_INET ;
    sock_addr.sin_addr.s_addr = inet_addr(addr) ;
    sock_addr.sin_port = htons(port) ;
    int res = bind(sock, (const struct sockaddr *)&sock_addr, sizeof(struct sockaddr));
    listen(sock,5);
    pthread_create(&nid_thread,NULL,thread_nid_server,NULL) ;

    return res == 0 ? MP_ROM_TRUE : MP_ROM_FALSE ;
}

STATIC mp_obj_t nid_deinit()
{
    close(sock) ;
    return MP_ROM_NONE ;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_nid_init,nid_init) ;
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_nid_deinit,nid_deinit) ;
DEFINE_PTR_OBJ(nid_input_read) ;

STATIC const mp_rom_map_elem_t mp_nid_globals_tab[] = {
        {MP_ROM_QSTR(MP_QSTR__init__), MP_ROM_QSTR(MP_QSTR_nid)},
        {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mp_nid_init)},
        {MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&mp_nid_deinit)},
        {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&PTR_OBJ(nid_input_read))}
};

STATIC MP_DEFINE_CONST_DICT(mp_nid_module_dict,mp_nid_globals_tab) ;

const mp_obj_module_t mp_module_nid = {
        .base = {&mp_type_module},
        .globals = (mp_obj_dict_t *)&mp_nid_module_dict,
};



