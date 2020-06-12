#include <stdio.h>
#include <stdlib.h>
#include "decoder.h"
#include "common.h"
#pragma comment(lib,"winmm.lib")

int main(int argc, char** argv)
{
	FILE* musicout;
	Bit_stream_struc bs;
	frame_params fr_ps;
	III_side_info_t III_side_info;
	III_scalefac_t III_scalefac;
	unsigned int old_crc;
	layer info;
	int sync, clip;
	int done = FALSE;
	unsigned long frameNum = 0;
	unsigned long bitsPerSlot;
	unsigned long sample_frames;

	typedef short PCM[2][SSLIMIT][SBLIMIT];
	PCM* pcm_sample;

	pcm_sample = (PCM*)mem_alloc((long)sizeof(PCM), (char*)"PCM Samp");

	fr_ps.header = &info;

	if ((fopen_s(&musicout,"musicout.pcm", "w+b")) != 0)
	{
		printf("无法创建文件：musicout.pcm\n");
		exit(1);
	}

	//创建比特流
	open_bit_stream_r(&bs, argv[1], BUFFER_SIZE);

	sample_frames = 0;
	while (!end_bs(&bs))
	{
		//查找同步头
		sync = seek_sync(&bs, SYNC_WORD, SYNC_WORD_LENGTH);
		if (!sync)
		{
			done = TRUE;
			printf("\n帧无法被定位\n");
			break;
		}

		//头信息提取
		decode_info(&bs, &fr_ps);
		hdr_to_frps(&fr_ps);
		frameNum++;
		if (info.error_protection)
			buffer_CRC(&bs, &old_crc);
		
		//判断是否是mp3
		switch (info.lay)
		{
		case 3:
		{
			int nSlots, main_data_end, flush_main;
			int bytes_to_discard, gr, ch, ss, sb;
			static int frame_start = 0;

			bitsPerSlot = 8;

			//读边信息
			III_get_side_info(&bs, &III_side_info, &fr_ps);


			//提取主信息
			nSlots = main_data_slots(fr_ps);

			for (; nSlots > 0; --nSlots)
			{
				hputbuf((unsigned int)getbits(&bs, 8), 8);
			}
			main_data_end = hsstell() / 8;

			if (flush_main = (hsstell() % 8))
			{
				hgetbits((int)(bitsPerSlot - flush_main));
				main_data_end++;
			}

			bytes_to_discard = frame_start - main_data_end - III_side_info.main_data_begin;
			if (main_data_end > 4096)
			{
				frame_start -= 4096;
				rewindNbytes(4096);
			}

			frame_start += main_data_slots(fr_ps);
			if (bytes_to_discard < 0)
			{
				frameNum--;
				break;
			}
			for (; bytes_to_discard > 0; --bytes_to_discard)
			{
				hgetbits(8);
			}

			clip = 0;
			for (gr = 0; gr < 2; gr++)
			{
				double lr[2][SBLIMIT][SSLIMIT], ro[2][SBLIMIT][SSLIMIT];
				for (ch = 0; ch < fr_ps.stereo; ch++)
				{
					long int is[SBLIMIT][SSLIMIT];
					int part2_start;
					part2_start = hsstell();

					//比例因子提取
					III_get_scale_factors(&III_scalefac, &III_side_info, gr, ch, &fr_ps);

					//hufman解码
					III_hufman_decode(is, &III_side_info, ch, gr, part2_start, &fr_ps);

					III_dequantize_sample(is, ro[ch], &III_scalefac, &(III_side_info.ch[ch].gr[gr]), ch, &fr_ps);
				}
				III_stereo(ro, lr, &III_scalefac, &(III_side_info.ch[0].gr[gr]), &fr_ps);
				for (ch = 0; ch < fr_ps.stereo; ch++)
				{
					double re[SBLIMIT][SSLIMIT];
					double hybridIn[SBLIMIT][SSLIMIT];  /* Hybrid filter input */
					double hybridOut[SBLIMIT][SSLIMIT]; /* Hybrid filter out */
					double polyPhaseIn[SBLIMIT];        /* PolyPhase Input. */
					III_reorder(lr[ch], re, &(III_side_info.ch[ch].gr[gr]), &fr_ps);
					//抗锯齿处理
					III_antialias(re, hybridIn, /* Antialias butterflies. */
						&(III_side_info.ch[ch].gr[gr]), &fr_ps);
					//IMDCT
					for (sb = 0; sb < SBLIMIT; sb++)
					{ /* Hybrid synthesis. */
						III_hybrid(hybridIn[sb], hybridOut[sb], sb, ch,
							&(III_side_info.ch[ch].gr[gr]), &fr_ps);
					}
					for (ss = 0; ss < 18; ss++) //多相频率倒置
						for (sb = 0; sb < SBLIMIT; sb++)
							if ((ss % 2) && (sb % 2))
								hybridOut[sb][ss] = -hybridOut[sb][ss];
					for (ss = 0; ss < 18; ss++)
					{ //多相合成
						for (sb = 0; sb < SBLIMIT; sb++)
							polyPhaseIn[sb] = hybridOut[sb][ss];
						//子带合成
						clip += SubBandSynthesis(polyPhaseIn, ch,
							&((*pcm_sample)[ch][ss][0]));
					}
				}
				//PCM 输出
				/* Output PCM sample points for one granule(颗粒). */
				out_fifo(*pcm_sample, 18, &fr_ps, done, musicout, &sample_frames);
			}
			if (clip > 0)
				printf("\n%d samples clipped.\n", clip);
		}
		break;
		default:
			printf("\nOnly layer III supported!\n");
			exit(1);
			break;
		}
	}
	close_bit_stream_r(&bs);
	fclose(musicout);
	printf("\nDecoding done.\n");
	return 0;


}
