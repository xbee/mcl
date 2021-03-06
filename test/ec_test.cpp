#define PUT(x) std::cout << #x "=" << (x) << std::endl
#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include <cybozu/benchmark.hpp>
#include <mcl/gmp_util.hpp>

#include <mcl/fp.hpp>
#include <mcl/ec.hpp>
#include <mcl/ecparam.hpp>
#include <time.h>
#include <math.h>

typedef mcl::FpT<> Fp;
struct tagZn;
typedef mcl::FpT<tagZn> Zn;
typedef mcl::EcT<Fp> Ec;

struct Test {
	const mcl::EcParam& para;
	Test(const mcl::EcParam& para, mcl::fp::Mode fpMode, mcl::ec::Mode ecMode)
		: para(para)
	{
		printf("fpMode=%s\n", mcl::fp::ModeToStr(fpMode));
		Fp::init(para.p, fpMode);
		Zn::init(para.n, fpMode);
		Ec::init(para.a, para.b, ecMode);
	}
	void cstr() const
	{
		Ec O;
		O.clear();
		CYBOZU_TEST_ASSERT(O.isZero());
		CYBOZU_TEST_ASSERT(O.isValid());
		Ec P;
		P.clear();
		Ec::neg(P, O);
		CYBOZU_TEST_EQUAL(P, O);
	}
	void pow2(Ec& Q, const Ec& P, int n) const
	{
		Q = P;
		for (int i = 0; i < n; i++) {
			Q += Q;
		}
	}
	void pow2test(const Ec& P, int n) const
	{
		Ec Q, R;
		pow2(Q, P, n);
		Q -= P; // Q = (2^n - 1)P
		Fp x = 1;
		for (int i = 0; i < n; i++) {
			x += x;
		}
		x -= 1; // x = 2^n - 1
		Ec::mul(R, P, x);
		CYBOZU_TEST_EQUAL(Q, R);
		Q = P;
		Ec::mul(Q, Q, x);
		CYBOZU_TEST_EQUAL(Q, R);
	}
	void ope() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Zn n = 0;
		CYBOZU_TEST_NO_EXCEPTION(Ec(x, y));
		CYBOZU_TEST_EXCEPTION(Ec(x, y + 1), cybozu::Exception);
		Ec P(x, y), Q, R, O;
		O.clear();
		CYBOZU_TEST_ASSERT(P.isNormalized());
		{
			Ec::neg(Q, P);
			CYBOZU_TEST_EQUAL(Q.x, P.x);
			CYBOZU_TEST_EQUAL(Q.y, -P.y);

			R = P + Q;
			CYBOZU_TEST_ASSERT(R.isZero());
			CYBOZU_TEST_ASSERT(R.isNormalized());
			CYBOZU_TEST_ASSERT(R.isValid());

			R = P + O;
			CYBOZU_TEST_EQUAL(R, P);
			R = O + P;
			CYBOZU_TEST_EQUAL(R, P);
		}

		{
			Ec::dbl(R, P);
			CYBOZU_TEST_ASSERT(!R.isNormalized());
			CYBOZU_TEST_ASSERT(R.isValid());
			Ec R2 = P + P;
			CYBOZU_TEST_EQUAL(R, R2);
			{
				Ec P2 = P;
				Ec::dbl(P2, P2);
				CYBOZU_TEST_EQUAL(P2, R2);
			}
			Ec R3L = R2 + P;
			Ec R3R = P + R2;
			CYBOZU_TEST_EQUAL(R3L, R3R);
			{
				Ec RR = R2;
				RR = RR + P;
				CYBOZU_TEST_EQUAL(RR, R3L);
				RR = R2;
				RR = P + RR;
				CYBOZU_TEST_EQUAL(RR, R3L);
				RR = P;
				RR = RR + RR;
				CYBOZU_TEST_EQUAL(RR, R2);
			}
			Ec::mul(R, P, 2);
			CYBOZU_TEST_EQUAL(R, R2);
			Ec R4L = R3L + R2;
			Ec R4R = R2 + R3L;
			CYBOZU_TEST_EQUAL(R4L, R4R);
			Ec::mul(R, P, 5);
			CYBOZU_TEST_EQUAL(R, R4L);
		}
		{
			R = P;
			for (int i = 0; i < 10; i++) {
				R += P;
			}
			Ec R2;
			Ec::mul(R2, P, 11);
			CYBOZU_TEST_EQUAL(R, R2);
		}
		Ec::mul(R, P, n - 1);
		CYBOZU_TEST_EQUAL(R, -P);
		R += P; // Ec::mul(R, P, n);
		CYBOZU_TEST_ASSERT(R.isZero());
		{
			const int tbl[] = { 1, 2, 63, 64, 65, 127, 128, 129 };
			for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
				pow2test(P, tbl[i]);
			}
		}
		{
			Ec::mul(Q, P, 0);
			CYBOZU_TEST_ASSERT(Q.isZero());
			Q = P;
			CYBOZU_TEST_ASSERT(!Q.isZero());
			Ec::mul(Q, Q, 0);
			CYBOZU_TEST_ASSERT(Q.isZero());
			Ec::mul(Q, P, 1);
			CYBOZU_TEST_EQUAL(P, Q);
		}
	}

	void mul() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		R.clear();
		for (int i = 0; i < 100; i++) {
			Ec::mul(Q, P, i);
			CYBOZU_TEST_EQUAL(Q, R);
			R += P;
		}
	}

	void neg_mul() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		R.clear();
		for (int i = 0; i < 100; i++) {
			Ec::mul(Q, P, -i);
			CYBOZU_TEST_EQUAL(Q, R);
			R -= P;
		}
	}
	void squareRoot() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		bool odd = y.isOdd();
		Fp yy;
		Ec::getYfromX(yy, x, odd);
		CYBOZU_TEST_EQUAL(yy, y);
		Fp::neg(y, y);
		odd = y.isOdd();
		yy.clear();
		Ec::getYfromX(yy, x, odd);
		CYBOZU_TEST_EQUAL(yy, y);
	}
	void mul_fp() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		Ec R;
		R.clear();
		for (int i = 0; i < 100; i++) {
			Ec::mul(Q, P, Zn(i));
			CYBOZU_TEST_EQUAL(Q, R);
			R += P;
		}
	}
	void str() const
	{
		const Fp x(para.gx);
		const Fp y(para.gy);
		Ec P(x, y);
		Ec Q;
		// not compressed
		Ec::setCompressedExpression(false);
		{
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		{
			Q.clear();
			CYBOZU_TEST_EQUAL(Q.getStr(), "0");
		}
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				int base = i == 0 ? 10 : 16;
				bool withPrefix = j == 0;
				int ioMode = base | (withPrefix ? mcl::IoPrefix : 0);
				std::string expected = "1 " + x.getStr(ioMode) + " " + y.getStr(ioMode);
				CYBOZU_TEST_EQUAL(P.getStr(ioMode), expected);
				std::ostringstream os;
				if (base == 16) {
					os << std::hex;
				}
				if (withPrefix) {
					os << std::showbase;
				}
				os << P;
				CYBOZU_TEST_EQUAL(os.str(), expected);
			}
		}
		{
			P = -P;
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		P.clear();
		{
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		CYBOZU_TEST_EXCEPTION_MESSAGE(P.setStr("1 3 5"), cybozu::Exception, "bad value");
		// compressed
		Ec::setCompressedExpression(true);
		P.set(x, y);
		{
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		{
			P = -P;
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		P.clear();
		{
			std::stringstream ss;
			ss << P;
			ss >> Q;
			CYBOZU_TEST_EQUAL(P, Q);
		}
		// IoFixedSizeByteSeq
		if (!Ec::isFixedSizeByteSeq()) return;
		P.set(x, y);
		{
			std::string s = P.getStr(mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(s.size(), Fp::getByteSize());
			Q.setStr(s, mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(P, Q);
		}
		{
			P = -P;
			std::string s = P.getStr(mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(s.size(), Fp::getByteSize());
			Q.setStr(s, mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(P, Q);
		}
		P.clear();
		{
			std::string s = P.getStr(mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(s.size(), Fp::getByteSize());
			CYBOZU_TEST_ASSERT(mcl::fp::isZeroArray(s.c_str(), s.size()));
			Q.setStr(s, mcl::IoFixedSizeByteSeq);
			CYBOZU_TEST_EQUAL(P, Q);
		}
	}
	void ioMode() const
	{
		const Fp x(para.gx);
		const Fp y(para.gy);
		Ec P(x, y);
		const mcl::IoMode tbl[] = {
			mcl::IoBin,
			mcl::IoDec,
			mcl::IoHex,
			mcl::IoArray,
			mcl::IoArrayRaw,
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			Fp::setIoMode(tbl[i]);
			{
				std::stringstream ss;
				ss << P;
				Ec Q;
				ss >> Q;
				CYBOZU_TEST_EQUAL(P, Q);
			}
			{
				std::stringstream ss;
				Ec Q;
				Q.clear();
				ss << Q;
				Ec R;
				ss >> R;
				CYBOZU_TEST_EQUAL(Q, R);
			}
		}
		Fp::setIoMode(mcl::IoAuto);
	}
	void mulCT() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y), Q1, Q2;
		for (int i = 0; i < 100; i++) {
			Zn r = i;
			Ec::mul(Q1, P, r);
			Ec::mulCT(Q2, P, r);
			CYBOZU_TEST_EQUAL(Q1, Q2);
		}
	}
	void compare() const
	{
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P1(x, y);
		Ec P2(x, -y);
		int c = Ec::compare(P1, P2);
		int cx = Fp::compare(y, -y);
		CYBOZU_TEST_EQUAL(c, cx);
		c = Ec::compare(P2, P1);
		cx = Fp::compare(-y, y);
		CYBOZU_TEST_EQUAL(c, cx);
		CYBOZU_TEST_EQUAL(Ec::compare(P1, P1), 0);
		bool b1, b2;
		b1 = P1 <= P2;
		b2 = y <= -y;
		CYBOZU_TEST_EQUAL(b1, b2);
		b1 = P1 < P2;
		b2 = y < -y;
		CYBOZU_TEST_EQUAL(b1, b2);
		CYBOZU_TEST_ASSERT(!(P1 < P1));
		CYBOZU_TEST_ASSERT((P1 <= P1));
	}

	template<class F>
	void test(F f, const char *msg) const
	{
		const int N = 300000;
		Fp x(para.gx);
		Fp y(para.gy);
		Ec P(x, y);
		Ec Q = P + P + P;
		clock_t begin = clock();
		for (int i = 0; i < N; i++) {
			f(Q, P, Q);
		}
		clock_t end = clock();
		printf("%s %.2fusec\n", msg, (end - begin) / double(CLOCKS_PER_SEC) / N * 1e6);
	}
/*
Affine : sandy-bridge
add 3.17usec
sub 2.43usec
dbl 3.32usec
mul 905.00usec
Jacobi
add 2.34usec
sub 2.65usec
dbl 1.56usec
mul 499.00usec
*/
	void run() const
	{
		cstr();
		ope();
		mul();
		neg_mul();
		mul_fp();
		squareRoot();
		str();
		ioMode();
		mulCT();
		compare();
	}
private:
	Test(const Test&);
	void operator=(const Test&);
};

void test_sub_sub(const mcl::EcParam& para, mcl::fp::Mode fpMode)
{
	puts("Proj");
	Test(para, fpMode, mcl::ec::Proj).run();
	puts("Jacobi");
	Test(para, fpMode, mcl::ec::Jacobi).run();
}

void test_sub(const mcl::EcParam *para, size_t paraNum)
{
	for (size_t i = 0; i < paraNum; i++) {
		puts(para[i].name);
		test_sub_sub(para[i], mcl::fp::FP_GMP);
#ifdef MCL_USE_LLVM
		test_sub_sub(para[i], mcl::fp::FP_LLVM);
		test_sub_sub(para[i], mcl::fp::FP_LLVM_MONT);
#endif
#ifdef MCL_USE_XBYAK
		test_sub_sub(para[i], mcl::fp::FP_XBYAK);
#endif
	}
}

int g_partial = -1;

CYBOZU_TEST_AUTO(all)
{
	if (g_partial & (1 << 3)) {
		const struct mcl::EcParam para3[] = {
	//		mcl::ecparam::p160_1,
			mcl::ecparam::secp160k1,
			mcl::ecparam::secp192k1,
			mcl::ecparam::NIST_P192,
		};
		test_sub(para3, CYBOZU_NUM_OF_ARRAY(para3));
	}

	if (g_partial & (1 << 4)) {
		const struct mcl::EcParam para4[] = {
			mcl::ecparam::secp224k1,
			mcl::ecparam::secp256k1,
			mcl::ecparam::NIST_P224,
			mcl::ecparam::NIST_P256,
		};
		test_sub(para4, CYBOZU_NUM_OF_ARRAY(para4));
	}

#if MCL_MAX_BIT_SIZE >= 384
	if (g_partial & (1 << 6)) {
		const struct mcl::EcParam para6[] = {
	//		mcl::ecparam::secp384r1,
			mcl::ecparam::NIST_P384,
		};
		test_sub(para6, CYBOZU_NUM_OF_ARRAY(para6));
	}
#endif

#if MCL_MAX_BIT_SIZE >= 521
	if (g_partial & (1 << 9)) {
		const struct mcl::EcParam para9[] = {
	//		mcl::ecparam::secp521r1,
			mcl::ecparam::NIST_P521,
		};
		test_sub(para9, CYBOZU_NUM_OF_ARRAY(para9));
	}
#endif
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		g_partial = -1;
	} else {
		g_partial = 0;
		for (int i = 1; i < argc; i++) {
			g_partial |= 1 << atoi(argv[i]);
		}
	}
	return cybozu::test::autoRun.run(argc, argv);
}
