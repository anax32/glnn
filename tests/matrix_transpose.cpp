#pragma comment (lib, "opengl32.lib")
#ifdef _DEBUG
#pragma comment (lib, "zlibstaticd.lib")
#pragma comment (lib, "libpng16_staticd.lib")
#else
#pragma comment (lib, "zlibstatic.lib")
#pragma comment (lib, "libpng16_static.lib")
#endif
#pragma comment (lib, "glew32s.lib")

#include <Windows.h>
#include <wingdi.h>

#include "gl\defs.h"
#include "gl\context.h"

#include <iostream>
#include <vector>

#include "glnnrt\gldnn_fs_rtt.h"
#include "glnnrt\gldnn_transpose.h"

int main(int argc, char** argv)
{
	if (gl::context::create_windowless() == false)
	{
		std::cout << "gl::context::create failed" << std::endl;
		return 1;
	}

	// create textures
	gl::texture::texture_set_t		textures;

  std::vector<float>	input(8 * 8);
  std::vector<float>	output(8 * 8);
  std::vector<float>	result(8 * 8);

	std::generate(
		std::begin(input),
		std::end(input),
		[i=0.0f]() mutable
		{
			return (i+=1.0f);
		});

  std::fill(
      std::begin(output),
      std::end(output),
      0.0f);

	textures["matrix"] = gl::texture::create(std::make_pair(8, 8));
	gl::texture::set_content(textures["matrix"], input, std::make_pair(8, 8));

	textures["output"] = gl::texture::create(std::make_pair(8, 8));
	gl::texture::set_content(textures["output"], output, std::make_pair(8, 8));

	{
		gldnn_transpose	transpose(textures["matrix"], textures["output"]);
		transpose();
	}

  gl::texture::extract(textures["output"], result);

  auto fails = 0u;

  for (auto i = 0; i < 8; i++)
  {
      for (auto j = 0; j < 8; j++)
      {
          auto in_idx = (i * 8) + j;
          auto ot_idx = (j * 8) + i;

          if (input[in_idx] != result[ot_idx])
          {
              fails++;
          }
      }
  }

	gl::context::clean(NULL);

  if (fails != 0)
      return fails;

	return 0;
}