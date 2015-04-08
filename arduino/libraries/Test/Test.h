#ifndef _TEST_H_
#define _TEST_H_

struct mysocket {
    void *mysock;
    size_t (*cb_send)(void *mysocket, void * data, size_t len);
    size_t (*cb_recv)(void *mysocket, void * data, size_t *len);
};

size_t send_smth(struct mysocket *msock) {
    return msock->cb_send(msock, (void*)"coucou", 6);
}

size_t recv_smth(struct mysocket *msock) {

    size_t len;
    void *data;;
    msock->cb_recv(msock, data, &len);
    return len;
}

#endif
