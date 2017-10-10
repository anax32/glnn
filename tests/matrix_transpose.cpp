#include "gl\defs.h"
#include "gl\context.h"

#include <iostream>
#include <vector>

#include "glnnrt\gldnn_transpose.h"

#include "test_bplate.h"

int main(int argc, char** argv)
{
  test_set_t    test_set (
  {
  {"transpose_square", []() -> bool {
      // create textures
      gl::texture::texture_set_t		textures;

      std::vector<float>	input(8 * 8);
      std::vector<float>	output(8 * 8);
      std::vector<float>	result(8 * 8);

      std::generate(
        std::begin(input),
        std::end(input),
        [i = 0.0f]() mutable
        {
          return (i += 1.0f);
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

      for (auto i = 0; i < 8; i++)
      {
          for (auto j = 0; j < 8; j++)
          {
              auto in_idx = (i * 8) + j;
              auto ot_idx = (j * 8) + i;

              if (input[in_idx] != result[ot_idx])
              {
                  return false;
              }
          }
      }

      return true;
  }}});

  if (gl::context::create_windowless() == false)
  {
      std::cout << "gl::context::create failed" << std::endl;
      return 1;
  }

  test_set.run();

  gl::context::clean(NULL);

	return test_set.fail_count();
}