#include "gldnn_fs_rtt.h"

#ifndef GLDNN_HADAMARD_H
#define GLDNN_HADAMARD_H
class gldnn_addition : public gldnn_fs_rtt
{
public:
    gldnn_addition()
        : gldnn_fs_rtt(
#include "shaders//transpose.vert"
            ,
#include "shaders//add.frag"
        )
    {}
    gldnn_addition(const unsigned int tex_id_A, const unsigned int tex_id_B, const unsigned int tex_id_O)
        : gldnn_fs_rtt(
#include "shaders//transpose.vert"
            ,
#include "shaders//add.frag"
            ,
            { tex_id_A, tex_id_B },
            tex_id_O
        )
    {}
    virtual ~gldnn_addition()
    {}
};
#endif