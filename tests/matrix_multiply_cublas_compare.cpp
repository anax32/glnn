#define NOMINMAX

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cublas_v2.h>

#include "gl\defs.h"
#include "gl\context.h"
#include "gl\texture.h"
#include "gl\framebuffer.h"
#include "gl\shader.h"
//#include "img_io\png_io.h"

#include <iostream>
#include <vector>
#include <iomanip>

#include "glnnrt\gldnn_fs_rtt.h"
#include "glnnrt\gldnn_multiply.h"

#include <cassert>
#include <thread>

template<typename T>
class mat_t
{
	typedef std::tuple<int, int>	mat_size_t;
public:
	mat_size_t		size;
	std::vector<T>	values;

	mat_t()
	{}
	mat_t(const mat_size_t& new_size)
		: size(new_size),
		  values(count())
	{
		std::fill(std::begin(values), std::end(values), 0.0f);
	}

	int rows() const { return std::get<0>(size); }
	int cols() const { return std::get<1>(size); }
	int count() const { return rows()*cols(); }

	typename std::vector<typename T>::const_iterator begin() const { return values.begin(); }
	typename std::vector<typename T>::const_iterator end() const { return values.end(); }
	typename std::vector<typename T>::iterator begin() { return values.begin(); }
	typename std::vector<typename T>::iterator end() { return values.end(); }
};

template<typename T>
class result_t
{
public:
	mat_t<T>	O;			// output matrix
	clock_t		allocate;	// allocation/setup time
	clock_t		execute;	// execution time
	clock_t		cleanup;	// cleanup time

	result_t(std::tuple<int,int> size)
		: O(size),
		  allocate(0),
		  execute(0),
		  cleanup(0)
	{}
};

template<typename T>
result_t<T> matmul_brute_force(const mat_t<T>& A, const mat_t<T>& B)
{
	auto		C = clock();
	result_t<T>	result (std::make_tuple(A.rows(), B.cols()));
	auto		I = A.rows();
	auto		J = B.cols();
	auto		K = A.cols();

	result.allocate = clock() - C;

	C = clock();
	for (auto i = 0; i < I; i++)
	{
		for (auto j = 0; j < J; j++)
		{
			for (auto k = 0; k < K; k++)
			{
				auto a_value = A.values[i + (k*I)];
				auto b_value = B.values[k + (j*K)];
				result.O.values[i + (j*I)] += a_value * b_value;
			}
		}
	}

	result.execute = clock() - C;
	result.cleanup = 0;

	return result;
}

template<typename T>
result_t<T> matmul_loop_inflation(const mat_t<T>& A, const mat_t<T>& B)
{
	auto		C = clock();
	result_t<T>	result (std::make_tuple(A.rows(), B.cols()));
	auto		I = A.rows();
	auto		J = B.cols();
	auto		K = A.cols();

	result.allocate = clock() - C;
	C = clock();
	for (auto i = 0; i < I*J*K; i++)
	{
		auto ij = i / K;
		auto i_ = (int)floor((float)ij / (float)J);
		auto j_ = ij % J;
		auto k_ = i % K;

		auto a_value = A.values[i_ + (k_*I)];
		auto b_value = B.values[k_ + (j_*K)];
		result.O.values[i_ + (j_*I)] += a_value * b_value;
	}

	result.execute = clock() - C;
	result.cleanup = 0;

	return result;
}

template<typename T>
void matmul_loop_inflation_threads_fn(const mat_t<T>& A,
									  const mat_t<T>& B,
									  mat_t<T>& O,
									  const std::tuple<unsigned int, unsigned int> range,
									  const std::tuple<unsigned int, unsigned int, unsigned int> IJK)
{
	const auto start = std::get<0>(range);
	const auto end = std::get<1>(range);
	const auto I = std::get<0>(IJK);
	const auto J = std::get<1>(IJK);
	const auto K = std::get<2>(IJK);

	for (auto i = start; i < end; i++)
	{
		auto ij = i / K;
		auto i_ = (int)floor((float)ij / (float)J);
		auto j_ = ij % J;
		auto k_ = i % K;

		auto a_value = A.values[i_ + (k_*I)];
		auto b_value = B.values[k_ + (j_*K)];
		O.values[i_ + (j_*I)] += a_value * b_value;
	}
}

template<typename T>
result_t<T> matmul_loop_inflation_threads(const mat_t<T>& A, const mat_t<T>& B, const unsigned int num_threads)
{
	auto		C = clock();
	result_t<T>	result (std::make_tuple(A.rows(), B.cols()));
	auto		I = A.rows();
	auto		J = B.cols();
	auto		K = A.cols();
	auto		N = I*J*K;
	
	result.allocate = clock() - C;
	C = clock();
	// create the threads
	if (num_threads == 1)
	{
		matmul_loop_inflation_threads_fn(A, B, result.O, std::make_tuple(0, N), std::make_tuple(I,J,K));
	}
	else
	{
		auto	T = 2;// num_threads;
		auto	thread_cnt = N / T;
		std::vector<std::thread>	threads;

		for (auto i = 0; i < N; i+=thread_cnt)
		{
			auto start = i;
			auto end = std::min(i+thread_cnt, N);

			threads.emplace_back(
				[&]() {
				matmul_loop_inflation_threads_fn(
					A,
					B,
					result.O,
					std::make_tuple(start, end),
					std::make_tuple(I, J, K)); }
			);
		}

		for (auto& t : threads)
		{
			t.join();
		}
	}
	result.execute = clock() - C;
	result.cleanup = 0;
	return result;
}

template<typename T>
result_t<T> matmul_gl(const mat_t<T>& A, const mat_t<T>& B)
{
	auto		C = clock();
	result_t<T>	result (std::make_tuple(A.rows(), B.cols()));
	auto		I = A.rows();
	auto		J = B.cols();
	auto		K = A.cols();
	gl::texture::texture_set_t		textures;

	// create textures
	textures["A"] = gl::texture::create(A.size, GL_R32F);
	textures["B"] = gl::texture::create(B.size, GL_R32F);
	textures["O"] = gl::texture::create(result.O.size, GL_R32F);

	gl::texture::set_content(textures["A"], A.values, A.size);
	gl::texture::set_content(textures["B"], B.values, B.size);
	gl::texture::set_content(textures["O"], result.O.values, result.O.size);

	gldnn_multiply	multiply(textures["A"], textures["B"], textures["O"]);
	multiply.bind_inputs({ textures["A"], textures["B"] });
	multiply.bind_output(textures["O"]);

	result.allocate = clock() - C;
	C = clock();

	multiply();

	result.execute = clock() - C;
	C = clock();

	gl::texture::extract(textures["O"], result.O.values);
	gl::texture::clean(textures);

	result.cleanup = clock() - C;
	return result;
}

/*
=======================================
CUDA/cuBLAS global variables!!
=======================================
*/
CUcontext	cuda_context;
cublasHandle_t	cublas_context;

template<typename T>
result_t<T> matmul_cublas(const mat_t<T>& A, const mat_t<T>& B)
{
	auto		C = clock();
	result_t<T>	result (std::make_tuple(A.rows(), B.cols()));
	auto		I = A.rows();
	auto		J = B.cols();
	auto		K = A.cols();

	float alpha = 1.0f;
	float beta = 0.0f;

	void	*A_device_storage = NULL;
	void	*B_device_storage = NULL;
	void	*O_device_storage = NULL;
	size_t	A_byte_length = A.values.size() * sizeof(T);
	size_t	B_byte_length = B.values.size() * sizeof(T);
	size_t	O_byte_length = result.O.values.size() * sizeof(T);

	if (cudaMalloc(&A_device_storage, A_byte_length) != cudaSuccess)
	{
		return result;
	}
	if (cudaMalloc(&B_device_storage, B_byte_length) != cudaSuccess)
	{
		cudaFree(A_device_storage);
		return result;
	}
	if (cudaMalloc(&O_device_storage, O_byte_length) != cudaSuccess)
	{
		cudaFree(B_device_storage);
		cudaFree(A_device_storage);
		return result;
	}

	// copy data to the cuda device
	cudaMemcpy(A_device_storage, A.values.data(), A_byte_length, cudaMemcpyHostToDevice);
	cudaMemcpy(B_device_storage, B.values.data(), B_byte_length, cudaMemcpyHostToDevice);
	//cudaMemcpy(O_device_storage, result.O.values.data(), O_byte_length, cudaMemcpyHostToDevice);
	cudaDeviceSynchronize();

	result.allocate = clock() - C;
	C = clock();

	auto cublas_result = cublasSgemm(
		cublas_context,
		CUBLAS_OP_N,
		CUBLAS_OP_N,
		I,	// rows a
		J,	// cols b
		K,	// cols a rows b
		&alpha,
		(float*)A_device_storage,
		I,
		(float*)B_device_storage,
		K,
		&beta,
		(float*)O_device_storage,
		I);

	cudaDeviceSynchronize();

	result.execute = clock() - C;
	C = clock();

	assert(cublas_result == CUBLAS_STATUS_SUCCESS);

	cudaMemcpy(result.O.values.data(), O_device_storage, O_byte_length, cudaMemcpyDeviceToHost);
	cudaDeviceSynchronize();

	cudaFree(A_device_storage);
	cudaFree(B_device_storage);
	cudaFree(O_device_storage);
	cudaDeviceSynchronize();

	result.cleanup = clock() - C;

	return result;
}

typedef enum algorithm_s
{
	BRUTE_FORCE				= 0x0001,
	INFLATION_LOOP			= 0x0002,
	INFLATION_ONE_THREAD    = 0x0004,
	INFLATION_FOUR_THREADS	= 0x0008,
	INFLATION_12_THREADS	= 0x0010,
	INFLATION_GL			= 0x0020,
	CUBLAS					= 0x0040
} algorithm_t;

void matmul_algo_comparison(const unsigned int I,
							const unsigned int J,
							const unsigned int K,
							int algorithms)
{
	mat_t<float>	A(std::make_tuple(I, K));
	mat_t<float>	B(std::make_tuple(K, J));
	mat_t<float>	E(std::make_tuple(I, J));

	auto input_value = 2.0f;	// fixed input
	auto expected_value = input_value*input_value*K;	// expected output

	std::fill(std::begin(A), std::end(A), input_value);
	std::fill(std::begin(B), std::end(B), input_value);
	std::fill(std::begin(E), std::end(E), expected_value);

	std::cout << std::fixed << std::setprecision(4);
	
	if (algorithms & BRUTE_FORCE)
	{
		auto R = matmul_brute_force(A, B);
		std::cout << "brute force: ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute  / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup  / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & INFLATION_LOOP)
	{
		auto R = matmul_loop_inflation(A, B);
		std::cout << "single loop: ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & INFLATION_ONE_THREAD)
	{
		auto R = matmul_loop_inflation_threads(A, B, 1);
		std::cout << "one  thread: ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & INFLATION_FOUR_THREADS)
	{
		auto R = matmul_loop_inflation_threads(A, B, 4);
		std::cout << "four thread: ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & INFLATION_12_THREADS)
	{
		auto R = matmul_loop_inflation_threads(A, B, 12);
		std::cout << "12   thread: ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & INFLATION_GL)
	{
		auto R = matmul_gl(A, B);
		std::cout << "gl         : ";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}

	if (algorithms & CUBLAS)
	{
		auto R = matmul_cublas(A, B);
		std::cout << "cublas      :";
		std::cout << (float)R.allocate / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.execute / CLOCKS_PER_SEC << ", ";
		std::cout << (float)R.cleanup / CLOCKS_PER_SEC << ", ";
		std::cout << (std::equal(std::begin(R.O), std::end(R.O), std::begin(E)) ? "PASS" : "FAIL") << std::endl;
	}
}

void matmul_algo_test(const unsigned int I, const unsigned int J, const unsigned int K)
{
	mat_t<float>	A (std::make_tuple(I, K));
	mat_t<float>	B (std::make_tuple(K, J));
	mat_t<float>	O (std::make_tuple(I, J));
	mat_t<float>	O_(std::make_tuple(I, J));

	auto input_value = 2.0f;	// fixed input
	auto expected_value = input_value*input_value*K;	// expected output

	std::fill(std::begin(A), std::end(A), input_value);
	std::fill(std::begin(B), std::end(B), input_value);
	std::fill(std::begin(O), std::end(O), 0.0f);
	std::fill(std::begin(O_), std::end(O_), 0.0f);

	unsigned int	ijk = 0;

	// basic brute force algorithm with a single index version mixed in
	// to compare in-process variable values as well as outputs
	for (auto i = 0u; i < I; i++)
	{
		for (auto j = 0u; j < J; j++)
		{
			for (auto k = 0u; k < K; k++, ijk++)
			{
				auto a_value = A.values[i + (k*I)];
				auto b_value = B.values[k + (j*K)];
				O.values[i + (j*I)] += a_value * b_value;

				// freep
				auto ij = ijk / K;
				auto i_ = (int)floor((float)ij / (float)J);
				auto j_ = ij % J;
				auto k_ = ijk % K;

				assert(i_ == i);
				assert(j_ == j);
				assert(k_ == k);

				O_.values[i_ + (j_*I)] += A.values[i_ + (k_*I)] * B.values[k_ + (j_*K)];
			}
		}
	}

	// check the brute force method produced the expected result
	assert(std::all_of(
		std::begin(O),
		std::end(O),
		[&expected_value](const float x) {return x == expected_value; }));

	// check the brute force and inline methods produce the same result
	assert(std::equal(
		std::begin(O),
		std::end(O),
		std::begin(O_)) == true);

	// perform the single loop algorithm in isolation
	std::fill(
		std::begin(O_),
		std::end(O_),
		0.0f);

	for (auto i = 0u; i < I*J*K; i++)
	{
		auto ij = i / K;
		auto i_ = (int)floor((float)ij / (float)J);
		auto j_ = ij % J;
		auto k_ = ijk % K;

		auto a_value = A.values[i_ + (k_*I)];
		auto b_value = B.values[k_ + (j_*K)];
		O_.values[i_ + (j_*I)] += a_value * b_value;
	}

	// check the results match the brute force output
	assert(std::equal(std::begin(O), std::end(O), std::begin(O_)) == true);
}

int main(int argc, char** argv)
{
	if (gl::context::create_windowless() == false)
	{
		std::cout << "gl::context::create failed" << std::endl;
		return 1;
	}
/*
	matmul_algo_test(4, 4, 4);
	matmul_algo_test(5, 5, 5);
	matmul_algo_test(3, 3, 3);
	matmul_algo_test(7, 8, 3);
	matmul_algo_test(8, 8, 8);
*/

	CUdevice	cuda_device;
	CUresult	cuda_result;
	cublasStatus_t	cublas_result;
	cuda_result = cuInit(0);
	cuda_result = cuDeviceGet(&cuda_device, 0);
	cuda_result = cuCtxCreate(&cuda_context, 0, cuda_device);
	cublas_result = cublasCreate(&cublas_context);

	auto ALL_ALGORITHMS = BRUTE_FORCE | INFLATION_LOOP | INFLATION_FOUR_THREADS | INFLATION_GL | CUBLAS;
	auto FAST_ALGORITHMS = INFLATION_GL | CUBLAS;

	std::cout << "4x4x4" << std::endl;
	matmul_algo_comparison(4, 4, 4, ALL_ALGORITHMS);
	std::cout << "5x5x5" << std::endl;
	matmul_algo_comparison(5, 5, 5, ALL_ALGORITHMS);
	std::cout << "3x3x3" << std::endl;
	matmul_algo_comparison(3, 3, 3, ALL_ALGORITHMS);
	std::cout << "7x8x3" << std::endl;
	matmul_algo_comparison(7, 8, 3, ALL_ALGORITHMS);
	std::cout << "1hx1hx1h" << std::endl;
	matmul_algo_comparison(100, 100, 100, FAST_ALGORITHMS);
	std::cout << "4hx4hx4h" << std::endl;
	matmul_algo_comparison(400, 400, 400, FAST_ALGORITHMS);
	std::cout << "4hx3hx5h" << std::endl;
	matmul_algo_comparison(400, 300, 500, FAST_ALGORITHMS);
	std::cout << "5hx5hx5h" << std::endl;
	matmul_algo_comparison(500, 500, 500, FAST_ALGORITHMS);
	std::cout << "6hx6hx6h" << std::endl;
	matmul_algo_comparison(600, 600, 600, FAST_ALGORITHMS);
	std::cout << "7hx7hx7h" << std::endl;
	matmul_algo_comparison(700, 700, 700, FAST_ALGORITHMS);
	std::cout << "8hx8hx8h" << std::endl;
	matmul_algo_comparison(800, 800, 800, FAST_ALGORITHMS);

	//std::cout << "1kx1kx1k" << std::endl;
	//matmul_algo_comparison(1000, 1000, 1000, FAST_ALGORITHMS);
	//
	//std::cout << "1024x1024x1024" << std::endl;
	//matmul_algo_comparison(1024, 1024, 1024, FAST_ALGORITHMS);
	//
	//std::cout << "4kx4kx4k" << std::endl;
	//matmul_algo_comparison(4000, 4000, 4000, FAST_ALGORITHMS);
	//
	//std::cout << "100x4kx100" << std::endl;
	//matmul_algo_comparison(100, 4000, 100, FAST_ALGORITHMS);

	//std::cout << "7kx8kx9k" << std::endl;
	//matmul_algo_comparison(7000, 8000, 9000, FAST_ALGORITHMS);

	cublas_result = cublasDestroy(cublas_context);
	cuda_result = cuCtxDetach(cuda_context);

	gl::context::clean(NULL);
	return 0;
}