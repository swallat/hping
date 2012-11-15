/* 
 * $smu-mark$ 
 * $name: statistics.c$ 
 * $author: Salvatore Sanfilippo <antirez@invece.org>$ 
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$ 
 * $license: This software is under GPL version 2 of license$ 
 * $date: Fri Nov  5 11:55:50 MET 1999$ 
 * $rev: 8$ 
 */ 

/* $Id: statistics.c,v 1.3 2004/04/09 23:38:56 antirez Exp $ */

#include <stdlib.h>
#include <stdio.h>

#include "hping2.h"
#include "globals.h"

void	print_statistics(int signal_id)
{
	unsigned int lossrate;

	close_pcap();
	if (recv_pkt > 0)
		lossrate = 100 - ((recv_pkt*100)/sent_pkt);
	else
		if (!sent_pkt)
			lossrate = 0;
		else
			lossrate = 100;


	double stdev_d = 0;
	double rtt_mean_d = 0;
	double unbiased_stdev = 0;
	if (rtt_counter > 0){
		mpf_t variance1, stdev1, count, mean, pow_mean, factor, sub;
		mpf_init(variance1);
		mpf_init(stdev1);
		mpf_init(mean);
		mpf_init(pow_mean);
		mpf_init(factor);
		mpf_init(sub);

		mpf_init_set_si(count, rtt_counter);
		mpf_div(mean, rtt_sum, count);
		mpf_pow_ui(pow_mean, mean, 2);
		mpf_mul(factor, pow_mean, count);
		mpf_sub(sub, rtt_sumsq, factor);
		mpf_div(variance1,sub, count);
		mpf_sqrt(stdev1, variance1);
		stdev_d = mpf_get_d(stdev1);

		rtt_mean_d = mpf_get_d(mean);
		if (rtt_counter > 1) {
			/*
			 * mean = sum/obs;
			 * variance = (1/obs-1) * (sumsq - obs*mean*mean);
			 */
			mpf_t variance, one, count_clean, divisor, stdev;
			mpf_init(variance);
			mpf_init(divisor);
			mpf_init(stdev);
			mpf_init_set_si(one, 1);
			mpf_init_set_si(count_clean, (rtt_counter-1));
			mpf_div(divisor, one, count_clean);
			mpf_mul(variance, divisor, sub);
			mpf_sqrt(stdev, variance);
			unbiased_stdev = mpf_get_d(stdev);
			mpf_clear(variance);
			mpf_clear(one);
			mpf_clear(count_clean);
			mpf_clear(divisor);
			mpf_clear(stdev);
		}
		mpf_clear(variance1);
		mpf_clear(stdev1);
		mpf_clear(count);
		mpf_clear(mean);
		mpf_clear(pow_mean);
		mpf_clear(factor);
		mpf_clear(sub);
	}

	fprintf(stderr, "\n--- %s hping statistic ---\n", targetname);
	fprintf(stderr, "%d packets tramitted, %d packets received, "
			"%d%% packet loss\n", sent_pkt, recv_pkt, lossrate);
	if (out_of_sequence_pkt)
		fprintf(stderr, "%d out of sequence packets received\n",
			out_of_sequence_pkt);
	fprintf(stderr, "round-trip min/avg/max/stdev/unbiased_stdev = %.4f/%.4f/%.4f/%.4f/%.4f ms\n",
		rtt_min, rtt_mean_d, rtt_max, stdev_d, unbiased_stdev);

	mpf_clear(rtt_sum);
	mpf_clear(rtt_sumsq);


	/* manage exit code */
	if (opt_tcpexitcode)
	{
		exit(tcp_exitcode);
	}
	else
	{
		if (recv_pkt)
			exit(0);
		else
			exit(1);
	}
};
