#define NOMINMAX

#include "gl\texture.h"
#include "gl\framebuffer.h"
#include "gl\shader.h"
#include <algorithm>

#ifndef GLDNN_FS_RTT_H
#define GLDNN_FS_RTT_H
class gldnn_fs_rtt
{
protected:
	gl::shader::definition_t	def_;
	gl::shader::shader_t		shader_;
	unsigned int				framebuffer_;
	std::vector<unsigned int>	inputs_;
	unsigned int				output_;
	gl::size2d					viewport_;

	void bind_input_textures(const std::vector<unsigned int>& textures) const
	{
		auto atu = GL_TEXTURE0;
		auto location = 0;
		auto texture_unit = 0;

		for (auto& input : inputs_)
		{
			glActiveTexture(atu++);
			glBindTexture(GL_TEXTURE_2D, input);
			glUniform1i(location++, texture_unit++);
		}

		// reset the texture unit
		glActiveTexture(GL_TEXTURE0);
	}

	// number of vertices to be fired off
	virtual size_t vertex_invocations() const
	{
		return 4;
	}

	// mode used to connect the vertices
	virtual GLenum vertex_mode() const
	{
		return GL_TRIANGLE_STRIP;
	}

	// draw command
	virtual void draw() const
	{
		glDrawArrays(
			vertex_mode(),
			0,
			static_cast<GLsizei>(vertex_invocations()));
		glFlush();
	}

	// setup/teardown some opengl state here
	virtual void pre_draw() const {};
	virtual void post_draw() const {};
	virtual void set_uniforms() const {};

public:
	gldnn_fs_rtt(const std::string& vertex_src,
               const std::string& fragment_src)
		: def_({ {"vertex", vertex_src}, {"fragment", fragment_src} }),
		  framebuffer_ (gl::framebuffer::create())
	{
		gl::shader::create(def_, shader_);
	}
	gldnn_fs_rtt(const std::string& vertex_src,
               const std::string& fragment_src,
               const unsigned int input,
               const unsigned int output)
		: gldnn_fs_rtt(vertex_src, fragment_src)
	{
		bind_input(input);
		bind_output(output);
	}
  gldnn_fs_rtt(const std::string& vertex_src,
               const std::string& fragment_src,
               const std::initializer_list<unsigned int>& inputs,
               const unsigned int output)
      : gldnn_fs_rtt(vertex_src, fragment_src)
  {
      bind_inputs(inputs);
      bind_output(output);
  }
	virtual ~gldnn_fs_rtt()
	{
		gl::framebuffer::clean(framebuffer_);
		gl::shader::clean(shader_);
	}
	void bind_input(const unsigned int new_input)
	{
		inputs_.clear();
		inputs_.push_back(new_input);
	}
	void bind_inputs(const std::vector<unsigned int>& new_inputs)
	{
		inputs_.clear();
		inputs_.resize(new_inputs.size());
		std::copy(
			std::begin(new_inputs),
			std::end(new_inputs),
			std::begin(inputs_));
	}
	void bind_output(const unsigned int new_output)
	{
		output_ = new_output;
		gl::texture::bind(output_);
		std::get<0>(viewport_) = gl::texture::width();
		std::get<1>(viewport_) = gl::texture::height();
	}
	auto operator()(void) const -> void
	{
		pre_draw();

		gl::framebuffer::bind(framebuffer_);
		gl::framebuffer::attach_texture(output_);

		if (gl::framebuffer::verify() == false)
		{
			std::cout << "ERR: framebuffer not valid" << std::endl;
		}

		if (gl::shader::link(shader_) == false)
		{
			std::cout << "ERR: shader not valid" << std::endl;
		}

		gl::shader::bind(shader_);

		glViewport(0, 0, std::get<0>(viewport_), std::get<1>(viewport_));

		// don't bother clearing
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bind_input_textures(inputs_);
		set_uniforms();

		draw();

		gl::shader::unbind();
		gl::framebuffer::unbind();

		post_draw();
	}
};
#endif