#include "gldnn_fs_rtt.h"

#ifndef GLDNN_MULTIPLY_H
#define GLDNN_MULTIPLY_H
class gldnn_multiply : public gldnn_fs_rtt
{
protected:
	unsigned int	I, J, K;

	virtual GLenum vertex_mode() const
	{
		return GL_POINTS;
	}
	size_t vertex_invocations() const
	{
		return I*J*K;
	}
#if 0
	void draw() const
	{
		glDrawArrays(vertex_mode (), 0, vertex_invocations ());
	}
#else
	void draw() const
	{
		auto mx = static_cast<GLsizei>(vertex_invocations());
		auto mode = vertex_mode();
		auto size = 1<<22;
		auto batch_size = size > mx ? mx : size;

#if 1
		auto offset_location = gl::shader::program_uniform_location(shader_, "offset");
		// for large matrices (5000x5000) we need to batch and flush the draw
		// commands, otherwise the pipeline gets clogged and windows resets the
		// display driver.
		// batch_size could probably be varied depending on hardware?
		auto i = 0;
		for (i = 0; i < mx-batch_size; i += batch_size)
		{
			glUniform1i(offset_location, i);
			glDrawArrays(mode, i, batch_size);
			glFlush();
		}

		glUniform1i(offset_location, i);
		glDrawArrays(mode, i, mx - i);
		glFlush();
#else
		glDrawArraysInstanced(mode, 0, size, mx / size);
#endif
	}
#endif
	void pre_draw() const
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	void post_draw() const
	{
		glDisable(GL_BLEND);
	}
	void set_uniforms() const
	{
		auto loc = gl::shader::program_uniform_location(shader_, "IJK");
		glUniform3f(static_cast<GLint>(loc), I, J, K);

    glUniform1i(0, 0);  // texture input A
    glUniform1i(1, 1);  // texture input B
	}
public:
	gldnn_multiply()
		: gldnn_fs_rtt(
#include "shaders//multiply.vert"
			,
#include "shaders//multiply.frag"
		)
	{}
	gldnn_multiply(const unsigned int tex_id_A, const unsigned int tex_id_B, const unsigned int tex_id_O)
      : gldnn_fs_rtt(
#include "shaders//multiply.vert"
          ,
#include "shaders//multiply.frag"
          ,
          { tex_id_A, tex_id_B },
          tex_id_O
      )
	{
		gl::texture::bind(tex_id_A);
		I = gl::texture::width();
		K = gl::texture::height();
		gl::texture::bind(tex_id_B);
		K = gl::texture::width();	// check same?
		J = gl::texture::height();
	}
	virtual ~gldnn_multiply()
	{}
};
#endif