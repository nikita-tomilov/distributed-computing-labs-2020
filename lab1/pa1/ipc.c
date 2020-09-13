#include "ipc.h"
#include <unistd.h>

int send(void * self, local_id dst, const Message * msg) {
    int* fds;
    int res = 0;
    int len = 0;

    if ((msg == NULL) || (self == NULL)) {
        return -1;
    }
    
    fds = (int*)self;
    
    if (fds[dst] == -1) {
        return -1;
    }
    
    len = sizeof(MessageHeader) + (msg->s_header.s_payload_len);
    res = write(fds[dst], msg, len);
    
    if ((res < 0) || (res != len)) {
        return -1;
    }
    return 0;
}

int send_multicast(void * self, const Message * msg) {
    int i = 0;
    int* fds;
    int res = 0;
    if ((msg == NULL) || (self == NULL)) {
        return -1;
    }

    fds = (int*)self;

    //loop until we reach border
    while (fds[i] != -2) {
        if (fds[i] != -1) {
            res = send(self, i, msg);
            if (res != 0) {
                return -1;
            }
        }
        i++;
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    MessageHeader msg_hdr;
    int* fds;
    int res;
    
    if ((msg == NULL) || (self == NULL)) {
        return -1;
    }
    
    fds = (int*)self;
    res = read(fds[from], &msg_hdr, sizeof(MessageHeader));
    
    if (res == sizeof(MessageHeader)) {
        msg->s_header = msg_hdr;
        res = read(fds[from], msg->s_payload, msg_hdr.s_payload_len);
        if (res == msg_hdr.s_payload_len) {
            return 0;
        } else {
            return -1;
        }
    }
    return 1;
}

int receive_any(void * self, Message * msg) {
    int i = 0;
    int res;
    int* fds;
    
    if ((msg == NULL) || (self == NULL)) {
        return -1;
    }
    
    fds = (int*)self;
    
    while (fds[i] != -2) {
        if (fds[i] != -1) {
            res = receive(self, i, msg);
            if (res != 1) {
                return res;
            }
        }
        i++;
    }
    return 1;
}
