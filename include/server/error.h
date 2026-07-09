#ifndef ERROR_H_
    #define ERROR_H_
    #include <stdbool.h>

// ERROR RETURN VALUE
    typedef struct {
        bool error;
        void *value;
        int nbPointer;
        char error_msg[256];
    } retour_t;

    retour_t fail(const char *error_msg, void *to_free);
    void free_error(retour_t rt);

#endif /* !ERROR_H_ */
