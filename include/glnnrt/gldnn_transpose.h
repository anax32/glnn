#include "gldnn_fs_rtt.h"

#ifndef GLDNN_TRANSPOSE_H
#define GLDNN_TRANSPOSE_H
class gldnn_transpose : public gldnn_fs_rtt
{
public:
    gldnn_transpose()
        : gldnn_fs_rtt(
#include "shaders//transpose.vert"
            ,
#include "shaders//transpose.frag"
        )
    {}
    gldnn_transpose(const unsigned int input_tex_id, const unsigned int output_tex_id)
        : gldnn_fs_rtt(
#include "shaders//transpose.vert"
            ,
#include "shaders//transpose.frag"
            ,
          input_tex_id,
          output_tex_id)
    {}
    virtual ~gldnn_transpose()
    {}
};
#endif