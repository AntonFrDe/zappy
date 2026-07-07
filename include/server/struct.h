/* ************************************************************************** */
/*                                                          LE - /            */
/*                                                              /             */
/*   struct.h                                         .::    .:/ .      .::   */
/*                                                 +:+:+   +:    +:  +:+:+    */
/*   By: anton <anton@student.le-101.fr>            +:+   +:    +:    +:+     */
/*                                                 #+#   #+    #+    #+#      */
/*   Created: 2026/07/07 17:27:57 by anton        #+#   ##    ##    #+#       */
/*   Updated: 2026/07/07 17:57:16 by anton       ###    #+. /#+    ###.fr     */
/*                                                         /                  */
/*                                                        /                   */
/* ************************************************************************** */



#ifndef STRUCT_H
    #define STRUCT_H
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>

// PARSING FLAG
    typedef struct {
        int port;
        uint64_t width;
        uint64_t height;
        char **teams;
        uint16_t teams_nb;
        uint16_t clients_nb;
        uint16_t freq;
    } flags_t;


// ERROR RETURN VALUE
    typedef struct {
        bool error;
        void *value;
        int nbPointer;
        char error_msg[256];
    } retour_t;

#endif // struct
