/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "crypto/cryptonight_aesni.h"

#include "xmrstak/misc/console.hpp"
#include "xmrstak/backend/iBackend.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/backend/cpu/cpuType.hpp"
#include "xmrstak/params.hpp"
#include "jconf.hpp"

#include "xmrstak/misc/executor.hpp"
#include "minethd.hpp"
#include "xmrstak/jconf.hpp"

#include "hwlocMemory.hpp"
#include "xmrstak/backend/miner_work.hpp"

#ifndef CONF_NO_HWLOC
#   include "autoAdjustHwloc.hpp"
#else
#   include "autoAdjust.hpp"
#endif

#include <assert.h>
#include <cmath>
#include <chrono>
#include <cstring>
#include <thread>
#include <bitset>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>

#if defined(__APPLE__)
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif //__APPLE__

#endif //_WIN32

namespace xmrstak
{
namespace cpu
{

bool minethd::thd_setaffinity(std::thread::native_handle_type h, uint64_t cpu_id)
{
#if defined(_WIN32)
	// we can only pin up to 64 threads
	if(cpu_id < 64)
	{
		return SetThreadAffinityMask(h, 1ULL << cpu_id) != 0;
	}
	else
	{
		printer::inst()->print_msg(L0, "WARNING: Windows supports only affinity up to 63.");
		return false;
	}
#elif defined(__APPLE__)
	thread_port_t mach_thread;
	thread_affinity_policy_data_t policy = { static_cast<integer_t>(cpu_id) };
	mach_thread = pthread_mach_thread_np(h);
	return thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1) == KERN_SUCCESS;
#elif defined(__FreeBSD__)
	cpuset_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	return pthread_setaffinity_np(h, sizeof(cpuset_t), &mn) == 0;
#elif defined(__OpenBSD__)
        printer::inst()->print_msg(L0,"WARNING: thread pinning is not supported under OPENBSD.");
        return true;
#else
	cpu_set_t mn;
	CPU_ZERO(&mn);
	CPU_SET(cpu_id, &mn);
	return pthread_setaffinity_np(h, sizeof(cpu_set_t), &mn) == 0;
#endif
}

minethd::minethd(miner_work& pWork, size_t iNo, int iMultiway, bool no_prefetch, int64_t affinity, const std::string& asm_version)
{
	this->backendType = iBackend::CPU;
	oWork = pWork;
	bQuit = 0;
	iThreadNo = (uint8_t)iNo;
	iJobNo = 0;
	bNoPrefetch = no_prefetch;
	this->affinity = affinity;
	asm_version_str = asm_version;

	std::unique_lock<std::mutex> lck(thd_aff_set);
	std::future<void> order_guard = order_fix.get_future();

	switch (iMultiway)
	{
	case 5:
		oWorkThd = std::thread(&minethd::penta_work_main, this);
		break;
	case 4:
		oWorkThd = std::thread(&minethd::quad_work_main, this);
		break;
	case 3:
		oWorkThd = std::thread(&minethd::triple_work_main, this);
		break;
	case 2:
		oWorkThd = std::thread(&minethd::double_work_main, this);
		break;
	case 1:
	default:
		oWorkThd = std::thread(&minethd::work_main, this);
		break;
	}

	order_guard.wait();

	if(affinity >= 0) //-1 means no affinity
		if(!thd_setaffinity(oWorkThd.native_handle(), affinity))
			printer::inst()->print_msg(L1, "WARNING setting affinity failed.");
}

cryptonight_ctx* minethd::minethd_alloc_ctx()
{
	cryptonight_ctx* ctx;
	alloc_msg msg = { 0 };

	switch (::jconf::inst()->GetSlowMemSetting())
	{
	case ::jconf::never_use:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (ctx == NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		else
		{
			ctx->hash_fn = nullptr;
			ctx->loop_fn = nullptr;
			ctx->fun_data = nullptr;
			ctx->asm_version = 0;
			ctx->last_algo = invalid_algo;
		}
		return ctx;

	case ::jconf::no_mlck:
		ctx = cryptonight_alloc_ctx(1, 0, &msg);
		if (ctx == NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		else
		{
			ctx->hash_fn = nullptr;
			ctx->loop_fn = nullptr;
			ctx->fun_data = nullptr;
			ctx->asm_version = 0;
			ctx->last_algo = invalid_algo;
		}
		return ctx;

	case ::jconf::print_warning:
		ctx = cryptonight_alloc_ctx(1, 1, &msg);
		if (msg.warning != NULL)
			printer::inst()->print_msg(L0, "MEMORY ALLOC FAILED: %s", msg.warning);
		if (ctx == NULL)
			ctx = cryptonight_alloc_ctx(0, 0, NULL);

		if (ctx != NULL)
		{
			ctx->hash_fn = nullptr;
			ctx->loop_fn = nullptr;
			ctx->fun_data = nullptr;
			ctx->asm_version = 0;
			ctx->last_algo = invalid_algo;
		}
		return ctx;

	case ::jconf::always_use:
		ctx = cryptonight_alloc_ctx(0, 0, NULL);

		ctx->hash_fn = nullptr;
		ctx->loop_fn = nullptr;
		ctx->fun_data = nullptr;
		ctx->asm_version = 0;
		ctx->last_algo = invalid_algo;

		return ctx;

	case ::jconf::unknown_value:
		return NULL; //Shut up compiler
	}

	return nullptr; //Should never happen
}

static constexpr size_t MAX_N = 5;
bool minethd::self_test()
{
	alloc_msg msg = { 0 };
	size_t res;
	bool fatal = false;

	switch (::jconf::inst()->GetSlowMemSetting())
	{
	case ::jconf::never_use:
		res = cryptonight_init(1, 1, &msg);
		fatal = true;
		break;

	case ::jconf::no_mlck:
		res = cryptonight_init(1, 0, &msg);
		fatal = true;
		break;

	case ::jconf::print_warning:
		res = cryptonight_init(1, 1, &msg);
		break;

	case ::jconf::always_use:
		res = cryptonight_init(0, 0, &msg);
		break;

	case ::jconf::unknown_value:
	default:
		return false; //Shut up compiler
	}

	if(msg.warning != nullptr)
		printer::inst()->print_msg(L0, "MEMORY INIT ERROR: %s", msg.warning);

	if(res == 0 && fatal)
		return false;

	cryptonight_ctx *ctx[MAX_N] = {0};
	for (int i = 0; i < MAX_N; i++)
	{
		if ((ctx[i] = minethd_alloc_ctx()) == nullptr)
		{
			printer::inst()->print_msg(L0, "ERROR: miner was not able to allocate memory.");
			for (int j = 0; j < i; j++)
				cryptonight_free_ctx(ctx[j]);
			return false;
		}
	}

	bool bResult = true;

	unsigned char out[32 * MAX_N];

	auto neededAlgorithms = ::jconf::inst()->GetCurrentCoinSelection().GetAllAlgorithms();

	for(const auto algo : neededAlgorithms)
	{
		bResult = true;

		if(!bResult)
			printer::inst()->print_msg(L0,
				"Cryptonight hash self-test failed. This might be caused by bad compiler optimizations.");
	}

	for (int i = 0; i < MAX_N; i++)
		cryptonight_free_ctx(ctx[i]);

	return bResult;
}

std::vector<iBackend*> minethd::thread_starter(uint32_t threadOffset, miner_work& pWork)
{
	std::vector<iBackend*> pvThreads;

	if(!configEditor::file_exist(params::inst().configFileCPU))
	{
		autoAdjust adjust;
		if(!adjust.printConfig())
			return pvThreads;
	}

	if(!jconf::inst()->parse_config())
	{
		win_exit();
	}


	//Launch the requested number of single and double threads, to distribute
	//load evenly we need to alternate single and double threads
	size_t i, n = jconf::inst()->GetThreadCount();
	pvThreads.reserve(n);

	jconf::thd_cfg cfg;
	for (i = 0; i < n; i++)
	{
		jconf::inst()->GetThreadConfig(i, cfg);

		if(cfg.iCpuAff >= 0)
		{
#if defined(__APPLE__)
			printer::inst()->print_msg(L1, "WARNING on macOS thread affinity is only advisory.");
#endif

			printer::inst()->print_msg(L1, "Starting %dx thread, affinity: %d.", cfg.iMultiway, (int)cfg.iCpuAff);
		}
		else
			printer::inst()->print_msg(L1, "Starting %dx thread, no affinity.", cfg.iMultiway);

		minethd* thd = new minethd(pWork, i + threadOffset, cfg.iMultiway, cfg.bNoPrefetch, cfg.iCpuAff, cfg.asm_version_str);
		pvThreads.push_back(thd);
	}

	return pvThreads;
}

/** get the supported asm name
 *
 * @return asm type based on the number of hashes per thread the internal
 *             evaluated cpu type
 */
static std::string getAsmName(const uint32_t num_hashes)
{
	std::string asm_type = "off";
	if(num_hashes != 0)
	{
		auto cpu_model = getModel();

		if(cpu_model.avx && cpu_model.aes)
		{
			if(cpu_model.type_name.find("Intel") != std::string::npos)
				asm_type = "intel_avx";
			else if(cpu_model.type_name.find("AMD") != std::string::npos)
				asm_type = "amd_avx";
		}
	}
	return asm_type;
}

template<size_t N>
void minethd::func_multi_selector(cryptonight_ctx** ctx, minethd::cn_on_new_job& on_new_job,
	bool bHaveAes, bool bNoPrefetch, const xmrstak_algo& algo, const std::string& asm_version_str)
{
	static_assert(N >= 1, "number of threads must be >= 1" );

	// We have two independent flag bits in the functions
	// therefore we will build a binary digit and select the
	// function as a two digit binary

	uint8_t algv;
	switch(algo.Id())
	{
	case cryptonight:
		algv = 2;
		break;
	case cryptonight_lite:
		algv = 1;
		break;
	case cryptonight_monero:
		algv = 0;
		break;
	case cryptonight_heavy:
		algv = 3;
		break;
	case cryptonight_aeon:
		algv = 4;
		break;
	case cryptonight_ipbc:
		algv = 5;
		break;
	case cryptonight_stellite:
		algv = 6;
		break;
	case cryptonight_masari:
		algv = 7;
		break;
	case cryptonight_haven:
		algv = 8;
		break;
	case cryptonight_bittube2:
		algv = 9;
		break;
	case cryptonight_monero_v8:
		algv = 10;
		break;
	case cryptonight_superfast:
		algv = 11;
		break;
	case cryptonight_gpu:
		algv = 12;
		break;
	case cryptonight_conceal:
		algv = 13;
		break;
	case cryptonight_r:
		algv = 14;
		break;
	case cryptonight_v8_reversewaltz:
		algv = 15;
		break;
	default:
		algv = 2;
		break;
	}

	static const cn_hash_fun func_table[] = {
		Cryptonight_hash<N>::template hash<cryptonight_monero, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_monero, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_monero, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_monero, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_lite, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_lite, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_lite, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_lite, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_heavy, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_heavy, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_heavy, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_heavy, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_aeon, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_aeon, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_aeon, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_aeon, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_ipbc, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_ipbc, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_ipbc, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_ipbc, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_stellite, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_stellite, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_stellite, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_stellite, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_masari, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_masari, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_masari, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_masari, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_haven, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_haven, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_haven, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_haven, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_bittube2, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_bittube2, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_bittube2, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_bittube2, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_monero_v8, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_monero_v8, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_monero_v8, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_monero_v8, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_superfast, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_superfast, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_superfast, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_superfast, true, true>,

		Cryptonight_hash_gpu::template hash<cryptonight_gpu, false, false>,
		Cryptonight_hash_gpu::template hash<cryptonight_gpu, true, false>,
		Cryptonight_hash_gpu::template hash<cryptonight_gpu, false, true>,
		Cryptonight_hash_gpu::template hash<cryptonight_gpu, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_conceal, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_conceal, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_conceal, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_conceal, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_r, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_r, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_r, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_r, true, true>,

		Cryptonight_hash<N>::template hash<cryptonight_v8_reversewaltz, false, false>,
		Cryptonight_hash<N>::template hash<cryptonight_v8_reversewaltz, true, false>,
		Cryptonight_hash<N>::template hash<cryptonight_v8_reversewaltz, false, true>,
		Cryptonight_hash<N>::template hash<cryptonight_v8_reversewaltz, true, true>
	};

	std::bitset<2> digit;
	digit.set(0, !bHaveAes);
	digit.set(1, !bNoPrefetch);

	ctx[0]->hash_fn = func_table[ algv << 2 | digit.to_ulong() ];

	// check for asm optimized version for cryptonight_v8
	if(algo == cryptonight_monero_v8)
	{
		std::string selected_asm = asm_version_str;
		if(selected_asm == "auto")
				selected_asm = cpu::getAsmName(N);

		if(selected_asm != "off")
		{
			patchAsmVariants<N>(selected_asm, ctx, algo);

			if(asm_version_str == "auto" && (selected_asm != "intel_avx" || selected_asm != "amd_avx"))
				printer::inst()->print_msg(L3, "Switch to assembler version for '%s' cpu's", selected_asm.c_str());
			else if(selected_asm != "intel_avx" && selected_asm != "amd_avx") // unknown asm type
				printer::inst()->print_msg(L1, "Assembler '%s' unknown, fallback to non asm version of cryptonight_v8", selected_asm.c_str());
		}
	}
	else if(algo == cryptonight_r && asm_version_str != "off")
	{
		std::string selected_asm = asm_version_str;
		if(selected_asm == "auto")
				selected_asm = cpu::getAsmName(N);
		if(selected_asm == "off")
		{
			for(int h = 0; h < N; ++h)
				ctx[h]->asm_version = 0;
		}
		else
		{
			printer::inst()->print_msg(L0, "enable cryptonight_r asm '%s' cpu's", selected_asm.c_str());
			for(int h = 0; h < N; ++h)
				ctx[h]->asm_version = selected_asm == "intel_avx" ? 1 : 2; // 1 == Intel; 2 == AMD
		}
	}

	for(int h = 1; h < N; ++h)
		ctx[h]->hash_fn = ctx[0]->hash_fn;

	static const std::unordered_map<uint32_t, minethd::cn_on_new_job> on_new_job_map = {
		{cryptonight_r, Cryptonight_R_generator<N>::template cn_on_new_job<cryptonight_r>},
	};

	auto it = on_new_job_map.find(algo.Id());
	if (it != on_new_job_map.end())
		on_new_job = it->second;
	else
		on_new_job = nullptr;
}

void minethd::func_selector(cryptonight_ctx** ctx, bool bHaveAes, bool bNoPrefetch, const xmrstak_algo& algo)
{
	minethd::cn_on_new_job dm;
	func_multi_selector<1>(ctx, dm, bHaveAes, bNoPrefetch, algo); // for testing us eauto, must be removed before the release
}

void minethd::work_main()
{
	multiway_work_main<1u>();
}

void minethd::double_work_main()
{
	multiway_work_main<2u>();
}

void minethd::triple_work_main()
{
	multiway_work_main<3u>();
}

void minethd::quad_work_main()
{
	multiway_work_main<4u>();
}

void minethd::penta_work_main()
{
	multiway_work_main<5u>();
}

template<size_t N>
void minethd::prep_multiway_work(uint8_t *bWorkBlob, uint32_t **piNonce)
{
	for (size_t i = 0; i < N; i++)
	{
		memcpy(bWorkBlob + oWork.iWorkSize * i, oWork.bWorkBlob, oWork.iWorkSize);
		if (i > 0)
			piNonce[i] = (uint32_t*)(bWorkBlob + oWork.iWorkSize * i + 39);
	}
}

template<uint32_t N>
void minethd::multiway_work_main()
{
	if(affinity >= 0) //-1 means no affinity
		bindMemoryToNUMANode(affinity);

	order_fix.set_value();
	std::unique_lock<std::mutex> lck(thd_aff_set);
	lck.release();
	std::this_thread::yield();

	cryptonight_ctx *ctx[MAX_N];
	uint64_t iCount = 0;
	uint64_t *piHashVal[MAX_N];
	uint32_t *piNonce[MAX_N];
	uint8_t bHashOut[MAX_N * 32];
	uint8_t bWorkBlob[sizeof(miner_work::bWorkBlob) * MAX_N];
	uint32_t iNonce;
	job_result res;

	for (size_t i = 0; i < N; i++)
	{
		ctx[i] = minethd_alloc_ctx();
		if(ctx[i] == nullptr)
		{
			printer::inst()->print_msg(L0, "ERROR: miner was not able to allocate memory.");
			for (int j = 0; j < i; j++)
				cryptonight_free_ctx(ctx[j]);
			win_exit(1);
		}
		piHashVal[i] = (uint64_t*)(bHashOut + 32 * i + 24);
		piNonce[i] = (i == 0) ? (uint32_t*)(bWorkBlob + 39) : nullptr;
	}

	if(!oWork.bStall)
		prep_multiway_work<N>(bWorkBlob, piNonce);

	globalStates::inst().iConsumeCnt++;

	// start with root algorithm and switch later if fork version is reached
	auto miner_algo = ::jconf::inst()->GetCurrentCoinSelection().GetDescription(1).GetMiningAlgoRoot();
	cn_on_new_job on_new_job;
	uint8_t version = 0;
	size_t lastPoolId = 0;

	func_multi_selector<N>(ctx, on_new_job, ::jconf::inst()->HaveHardwareAes(), bNoPrefetch, miner_algo, asm_version_str);
	while (bQuit == 0)
	{
		if (oWork.bStall)
		{
			/*	We are stalled here because the executor didn't find a job for us yet,
			either because of network latency, or a socket problem. Since we are
			raison d'etre of this software it us sensible to just wait until we have something*/

			while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			globalStates::inst().consume_work(oWork, iJobNo);
			prep_multiway_work<N>(bWorkBlob, piNonce);
			continue;
		}

		constexpr uint32_t nonce_chunk = 4096;
		int64_t nonce_ctr = 0;

		assert(sizeof(job_result::sJobID) == sizeof(pool_job::sJobID));

		if(oWork.bNiceHash)
			iNonce = *piNonce[0];

		uint8_t new_version = oWork.getVersion();
		if(new_version != version || oWork.iPoolId != lastPoolId)
		{
			coinDescription coinDesc = ::jconf::inst()->GetCurrentCoinSelection().GetDescription(oWork.iPoolId);
			if(new_version >= coinDesc.GetMiningForkVersion())
			{
				miner_algo = coinDesc.GetMiningAlgo();
				func_multi_selector<N>(ctx, on_new_job, ::jconf::inst()->HaveHardwareAes(), bNoPrefetch, miner_algo, asm_version_str);
			}
			else
			{
				miner_algo = coinDesc.GetMiningAlgoRoot();
				func_multi_selector<N>(ctx, on_new_job, ::jconf::inst()->HaveHardwareAes(), bNoPrefetch, miner_algo, asm_version_str);
			}
			lastPoolId = oWork.iPoolId;
			version = new_version;
		}

		if(on_new_job != nullptr)
			on_new_job(oWork, ctx);

		while (globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) == iJobNo)
		{
			if ((iCount++ & 0x7) == 0)  //Store stats every 8*N hashes
			{
				uint64_t iStamp = get_timestamp_ms();
				iHashCount.store(iCount * N, std::memory_order_relaxed);
				iTimestamp.store(iStamp, std::memory_order_relaxed);
			}

			nonce_ctr -= N;
			if(nonce_ctr <= 0)
			{
				globalStates::inst().calc_start_nonce(iNonce, oWork.bNiceHash, nonce_chunk);
				nonce_ctr = nonce_chunk;
				// check if the job is still valid, there is a small posibility that the job is switched
				if(globalStates::inst().iGlobalJobNo.load(std::memory_order_relaxed) != iJobNo)
					break;
			}

			for (size_t i = 0; i < N; i++)
				*piNonce[i] = iNonce++;

			ctx[0]->hash_fn(bWorkBlob, oWork.iWorkSize, bHashOut, ctx, miner_algo);

			for (size_t i = 0; i < N; i++)
			{
				if (*piHashVal[i] < oWork.iTarget)
				{
					executor::inst()->push_event(
						ex_event(job_result(oWork.sJobID, iNonce - N + i, bHashOut + 32 * i, iThreadNo, miner_algo),
						oWork.iPoolId)
					);
				}
			}

			std::this_thread::yield();
		}

		globalStates::inst().consume_work(oWork, iJobNo);
		prep_multiway_work<N>(bWorkBlob, piNonce);
	}

	for (int i = 0; i < N; i++)
		cryptonight_free_ctx(ctx[i]);
}

} // namespace cpu
} // namespace xmrstak
