#include "webrtc/typedefs.h"

#if defined(WEBRTC_HAS_NEON)

#include "aec_rdft_new.h"
#include <arm_neon.h>

namespace youme{
namespace webrtc_new {
		
static const int kBrv_64_Table[] =
{
2, 32, 4, 64, 6, 96, 10, 40, 12, 72, 14, 104, 18, 48, 20, 80,
22, 112, 26, 56, 28, 88, 30, 120, 36, 66, 38, 98, 44, 74, 46, 106,
52, 82, 54, 114, 60, 90, 62, 122, 70, 100, 78, 108, 86, 116, 94, 124,
};

ALIGN16_BEG static const float ALIGN16_END kOrder2_Table[] =
{
1.000000000f, 0.000000000f, 0.923879533f, 0.382683432f, 0.707106781f, 0.707106781f, 0.382683432f, 0.923879533f, 
1.000000000f, 0.000000000f, 0.707106781f, 0.707106781f, 0.000000000f, 1.000000000f, -0.707106781f, 0.707106781f, 
1.000000000f, 0.000000000f, 0.382683432f, 0.923879533f, -0.707106781f, 0.707106781f, -0.923879533f, -0.382683432f, 
};


ALIGN16_BEG static const float ALIGN16_END kOrder3_Table[] = 
{
1.000000000f, 0.000000000f, 0.995184727f, 0.098017140f, 0.980785280f, 0.195090322f, 0.956940336f, 0.290284677f, 
0.923879533f, 0.382683432f, 0.881921264f, 0.471396737f, 0.831469612f, 0.555570233f, 0.773010453f, 0.634393284f, 
0.707106781f, 0.707106781f, 0.634393284f, 0.773010453f, 0.555570233f, 0.831469612f, 0.471396737f, 0.881921264f, 
0.382683432f, 0.923879533f, 0.290284677f, 0.956940336f, 0.195090322f, 0.980785280f, 0.098017140f, 0.995184727f, 
1.000000000f, 0.000000000f, 0.980785280f, 0.195090322f, 0.923879533f, 0.382683432f, 0.831469612f, 0.555570233f, 
0.707106781f, 0.707106781f, 0.555570233f, 0.831469612f, 0.382683432f, 0.923879533f, 0.195090322f, 0.980785280f, 
0.000000000f, 1.000000000f, -0.195090322f, 0.980785280f, -0.382683432f, 0.923879533f, -0.555570233f, 0.831469612f, 
-0.707106781f, 0.707106781f, -0.831469612f, 0.555570233f, -0.923879533f, 0.382683432f, -0.980785280f, 0.195090322f, 
1.000000000f, 0.000000000f, 0.956940336f, 0.290284677f, 0.831469612f, 0.555570233f, 0.634393284f, 0.773010453f, 
0.382683432f, 0.923879533f, 0.098017140f, 0.995184727f, -0.195090322f, 0.980785280f, -0.471396737f, 0.881921264f, 
-0.707106781f, 0.707106781f, -0.881921264f, 0.471396737f, -0.980785280f, 0.195090322f, -0.995184727f, -0.098017140f, 
-0.923879533f, -0.382683432f, -0.773010453f, -0.634393284f, -0.555570233f, -0.831469612f, -0.290284677f, -0.956940336f, 
};

ALIGN16_BEG static const float ALIGN16_END kSwap_Table[] =
{
1.0f, -1.0f,
};

static void bitrv4_128_neon(float* a) {
  int i;
  float * pp, * pq;
  float32x2_t swap_p, swap_q;

  for(i = 0; i < 48; i += 2){
    pp = &a[kBrv_64_Table[i]];
    pq = &a[kBrv_64_Table[i + 1]];
    swap_p = vld1_f32(pp);
    swap_q = vld1_f32(pq);
    vst1_f32(pp, swap_q);
    vst1_f32(pq, swap_p);
  }
}


static void order1(float * a){
	int i, j;
	float32x2_t in1, in2, in3, in4; 
	float32x2_t tmp, tmp1, tmp2;

    tmp = vld1_f32(&kSwap_Table[0]);
    for(i = 0, j = 0; i < 16; i++, j += 8){
		in1 = vld1_f32(&a[j + 0]);
		in2 = vld1_f32(&a[j + 2]);
		in3 = vld1_f32(&a[j + 4]);
		in4 = vld1_f32(&a[j + 6]);

		tmp1 = vadd_f32(in1, in3);
		tmp2 = vadd_f32(in2, in4);
		vst1_f32(&a[j + 0], vadd_f32(tmp1, tmp2));
		vst1_f32(&a[j + 4], vsub_f32(tmp1, tmp2));

		in2 = vrev64_f32(vmul_f32(in2, tmp));
		in4 = vrev64_f32(vmul_f32(in4, tmp));
		tmp1 = vsub_f32(in1, in3);
		tmp2 = vsub_f32(in2, in4);
		//vst1_f32(&a[j + 2], vsub_f32(tmp1, tmp2));
        vst1_f32(&a[j + 2], vadd_f32(tmp1, tmp2));
		//vst1_f32(&a[j + 6], vadd_f32(tmp1, tmp2));
        vst1_f32(&a[j + 6], vsub_f32(tmp1, tmp2));
	}
}

static void order2(float * a){
	int i, j;
	float32x4x2_t io1, io2, io3, io4;
	float32x4x2_t wn2, wn3, wn4;
	float32x4_t tmp1r, tmp1i, tmp2r, tmp2i, tmp3r, tmp3i, tmp4r, tmp4i;

	wn2 = vld2q_f32(&kOrder2_Table[0]);
	wn3 = vld2q_f32(&kOrder2_Table[8]);
	wn4 = vld2q_f32(&kOrder2_Table[16]);

	for(i = 0, j = 0; i < 4; i++, j += 32){
		io1 = vld2q_f32(&a[j + 0]);
		io2 = vld2q_f32(&a[j + 8]);
		io3 = vld2q_f32(&a[j + 16]);
		io4 = vld2q_f32(&a[j + 24]);

		tmp1r = io1.val[0];
		tmp1i = io1.val[1];
		tmp2r = vsubq_f32(vmulq_f32(io2.val[0], wn2.val[0]), 
			vmulq_f32(io2.val[1], wn2.val[1]));
		tmp2i = vaddq_f32(vmulq_f32(io2.val[1], wn2.val[0]), 
			vmulq_f32(io2.val[0], wn2.val[1]));
		tmp3r = vsubq_f32(vmulq_f32(io3.val[0], wn3.val[0]), 
			vmulq_f32(io3.val[1], wn3.val[1]));
		tmp3i = vaddq_f32(vmulq_f32(io3.val[1], wn3.val[0]), 
			vmulq_f32(io3.val[0], wn3.val[1]));
		tmp4r = vsubq_f32(vmulq_f32(io4.val[0], wn4.val[0]), 
			vmulq_f32(io4.val[1], wn4.val[1]));
		tmp4i = vaddq_f32(vmulq_f32(io4.val[1], wn4.val[0]), 
			vmulq_f32(io4.val[0], wn4.val[1]));

		io1.val[0] = vaddq_f32(vaddq_f32(tmp1r, tmp2r),
			vaddq_f32(tmp3r, tmp4r));
		io1.val[1] = vaddq_f32(vaddq_f32(tmp1i, tmp2i),
			vaddq_f32(tmp3i, tmp4i));
		//io2.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
		//	vaddq_f32(tmp3r, tmp4i));
		//io2.val[1] = vsubq_f32(vsubq_f32(tmp1i, tmp2r),
		//	vsubq_f32(tmp3i, tmp4r));
        io2.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
        	vsubq_f32(tmp3r, tmp4i));
        io2.val[1] = vsubq_f32(vaddq_f32(tmp1i, tmp2r),
        	vaddq_f32(tmp3i, tmp4r));
        
		io3.val[0] = vaddq_f32(vsubq_f32(tmp1r, tmp2r),
			vsubq_f32(tmp3r, tmp4r));
		io3.val[1] = vaddq_f32(vsubq_f32(tmp1i, tmp2i),
			vsubq_f32(tmp3i, tmp4i));
		//io4.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
		//	vsubq_f32(tmp3r, tmp4i));
		//io4.val[1] = vsubq_f32(vaddq_f32(tmp1i, tmp2r),
		//	vaddq_f32(tmp3i, tmp4r));
        io4.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
        	vaddq_f32(tmp3r, tmp4i));
        io4.val[1] = vsubq_f32(vsubq_f32(tmp1i, tmp2r),
        	vsubq_f32(tmp3i, tmp4r));

		vst2q_f32(&a[j + 0], io1);
		vst2q_f32(&a[j + 8], io2);
		vst2q_f32(&a[j + 16], io3);
		vst2q_f32(&a[j + 24], io4);
	}
}

static void iorder3(float * a){
	int i, j;
	float32x4x2_t io1, io2, io3, io4;
	float32x4x2_t wn2, wn3, wn4;
	float32x4_t tmp1r, tmp1i, tmp2r, tmp2i, tmp3r, tmp3i, tmp4r, tmp4i;

	for(i = 0, j = 0; i < 4; i++, j += 8){
		wn2 = vld2q_f32(&kOrder3_Table[j + 0]);
		wn3 = vld2q_f32(&kOrder3_Table[j + 32]);
		wn4 = vld2q_f32(&kOrder3_Table[j + 64]);

		io1 = vld2q_f32(&a[j + 0]);
		io2 = vld2q_f32(&a[j + 32]);
		io3 = vld2q_f32(&a[j + 64]);
		io4 = vld2q_f32(&a[j + 96]);

		tmp1r = io1.val[0];
		tmp1i = io1.val[1];
		tmp2r = vsubq_f32(vmulq_f32(io2.val[0], wn2.val[0]), 
			vmulq_f32(io2.val[1], wn2.val[1]));
		tmp2i = vaddq_f32(vmulq_f32(io2.val[1], wn2.val[0]), 
			vmulq_f32(io2.val[0], wn2.val[1]));
		tmp3r = vsubq_f32(vmulq_f32(io3.val[0], wn3.val[0]), 
			vmulq_f32(io3.val[1], wn3.val[1]));
		tmp3i = vaddq_f32(vmulq_f32(io3.val[1], wn3.val[0]), 
			vmulq_f32(io3.val[0], wn3.val[1]));
		tmp4r = vsubq_f32(vmulq_f32(io4.val[0], wn4.val[0]), 
			vmulq_f32(io4.val[1], wn4.val[1]));
		tmp4i = vaddq_f32(vmulq_f32(io4.val[1], wn4.val[0]), 
			vmulq_f32(io4.val[0], wn4.val[1]));

        io1.val[0] = vaddq_f32(vaddq_f32(tmp1r, tmp2r),
                               vaddq_f32(tmp3r, tmp4r));
        io1.val[1] = vnegq_f32(vaddq_f32(vaddq_f32(tmp1i, tmp2i),
                               vaddq_f32(tmp3i, tmp4i)));
        //io2.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
        //	vaddq_f32(tmp3r, tmp4i));
        //io2.val[1] = vsubq_f32(vsubq_f32(tmp1i, tmp2r),
        //	vsubq_f32(tmp3i, tmp4r));
        io2.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
                               vsubq_f32(tmp3r, tmp4i));
        io2.val[1] = vnegq_f32(vsubq_f32(vaddq_f32(tmp1i, tmp2r),
                               vaddq_f32(tmp3i, tmp4r)));
        
        io3.val[0] = vaddq_f32(vsubq_f32(tmp1r, tmp2r),
                               vsubq_f32(tmp3r, tmp4r));
        io3.val[1] = vnegq_f32(vaddq_f32(vsubq_f32(tmp1i, tmp2i),
                               vsubq_f32(tmp3i, tmp4i)));
        //io4.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
        //	vsubq_f32(tmp3r, tmp4i));
        //io4.val[1] = vsubq_f32(vaddq_f32(tmp1i, tmp2r),
        //	vaddq_f32(tmp3i, tmp4r));
        io4.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
                               vaddq_f32(tmp3r, tmp4i));
        io4.val[1] = vnegq_f32(vsubq_f32(vsubq_f32(tmp1i, tmp2r),
                               vsubq_f32(tmp3i, tmp4r)));

		vst2q_f32(&a[j + 0], io1);
		vst2q_f32(&a[j + 32], io2);
		vst2q_f32(&a[j + 64], io3);
		vst2q_f32(&a[j + 96], io4);
	}
}

static void order3(float * a){
	int i, j;
	float32x4x2_t io1, io2, io3, io4;
	float32x4x2_t wn2, wn3, wn4;
	float32x4_t tmp1r, tmp1i, tmp2r, tmp2i, tmp3r, tmp3i, tmp4r, tmp4i;

	for(i = 0, j = 0; i < 4; i++, j += 8){
		wn2 = vld2q_f32(&kOrder3_Table[j + 0]);
		wn3 = vld2q_f32(&kOrder3_Table[j + 32]);
		wn4 = vld2q_f32(&kOrder3_Table[j + 64]);

		io1 = vld2q_f32(&a[j + 0]);
		io2 = vld2q_f32(&a[j + 32]);
		io3 = vld2q_f32(&a[j + 64]);
		io4 = vld2q_f32(&a[j + 96]);

		tmp1r = io1.val[0];
		tmp1i = io1.val[1];
		tmp2r = vsubq_f32(vmulq_f32(io2.val[0], wn2.val[0]), 
			vmulq_f32(io2.val[1], wn2.val[1]));
		tmp2i = vaddq_f32(vmulq_f32(io2.val[1], wn2.val[0]), 
			vmulq_f32(io2.val[0], wn2.val[1]));
		tmp3r = vsubq_f32(vmulq_f32(io3.val[0], wn3.val[0]), 
			vmulq_f32(io3.val[1], wn3.val[1]));
		tmp3i = vaddq_f32(vmulq_f32(io3.val[1], wn3.val[0]), 
			vmulq_f32(io3.val[0], wn3.val[1]));
		tmp4r = vsubq_f32(vmulq_f32(io4.val[0], wn4.val[0]), 
			vmulq_f32(io4.val[1], wn4.val[1]));
		tmp4i = vaddq_f32(vmulq_f32(io4.val[1], wn4.val[0]), 
			vmulq_f32(io4.val[0], wn4.val[1]));

        io1.val[0] = vaddq_f32(vaddq_f32(tmp1r, tmp2r),
                               vaddq_f32(tmp3r, tmp4r));
        io1.val[1] = vaddq_f32(vaddq_f32(tmp1i, tmp2i),
                               vaddq_f32(tmp3i, tmp4i));
        //io2.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
        //	vaddq_f32(tmp3r, tmp4i));
        //io2.val[1] = vsubq_f32(vsubq_f32(tmp1i, tmp2r),
        //	vsubq_f32(tmp3i, tmp4r));
        io2.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
                               vsubq_f32(tmp3r, tmp4i));
        io2.val[1] = vsubq_f32(vaddq_f32(tmp1i, tmp2r),
                               vaddq_f32(tmp3i, tmp4r));
        
        io3.val[0] = vaddq_f32(vsubq_f32(tmp1r, tmp2r),
                               vsubq_f32(tmp3r, tmp4r));
        io3.val[1] = vaddq_f32(vsubq_f32(tmp1i, tmp2i),
                               vsubq_f32(tmp3i, tmp4i));
        //io4.val[0] = vsubq_f32(vsubq_f32(tmp1r, tmp2i),
        //	vsubq_f32(tmp3r, tmp4i));
        //io4.val[1] = vsubq_f32(vaddq_f32(tmp1i, tmp2r),
        //	vaddq_f32(tmp3i, tmp4r));
        io4.val[0] = vsubq_f32(vaddq_f32(tmp1r, tmp2i),
                               vaddq_f32(tmp3r, tmp4i));
        io4.val[1] = vsubq_f32(vsubq_f32(tmp1i, tmp2r),
                               vsubq_f32(tmp3i, tmp4r));

		vst2q_f32(&a[j + 0], io1);
		vst2q_f32(&a[j + 32], io2);
		vst2q_f32(&a[j + 64], io3);
		vst2q_f32(&a[j + 96], io4);
	}
}

void rdft_b4(float * a){
	bitrv4_128_neon(a);
	order1(a);
	order2(a);
	order3(a);
}

void irdft_b4(float * a){
	bitrv4_128_neon(a);
	order1(a);
	order2(a);
	iorder3(a);
}

}  // namespace webrtc_new

} //namespace youme

#endif