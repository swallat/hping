/* 
 * $smu-mark$ 
 * $name: rtt.c$ 
 * $author: Salvatore Sanfilippo <antirez@invece.org>$ 
 * $copyright: Copyright (C) 1999 by Salvatore Sanfilippo$ 
 * $license: This software is under GPL version 2 of license$ 
 * $date: Fri Nov  5 11:55:49 MET 1999$ 
 * $rev: 3$ 
 */ 

/* $Id: rtt.c,v 1.2 2003/09/01 00:22:06 antirez Exp $ */

#include <time.h>
#include <stdio.h>

#include "hping2.h"
#include "globals.h"

int minavgmax_history(float ms_delay) {
	static int history_count = 0;
	static float rtt_avg_history = 0;

	float diff = rtt_avg_history * 8; // 800 percent
	float min_diff = (rtt_avg_history-diff);
	float max_diff = (rtt_avg_history+diff);

	if (history_count < 50 || (ms_delay <= max_diff && ms_delay >= min_diff)) { // Use 50 measures to create history
		if (history_count < 10000) { // Use maximum 10000 measures for history
				history_count++;
		}
		rtt_avg_history = (rtt_avg_history*(history_count-1)/history_count)+(ms_delay/history_count);
		history_accepted++;
		return 1;
	} else {
		history_dropped++;
		return 0; //refuse delay
	}

}

int check_pre_run_time(time_t curTime) {
	if (opt_use_pre_time && (curTime - initTime_sec) < opt_pre_run_time) {
		return 0; //refuse
	} else {
		return 1;
	}
}

void minavgmax(float ms_delay, time_t curTime)
{
	//static int avg_counter = 0;

	if (check_pre_run_time(curTime) == 0) {
		return;
	}
	if (minavgmax_history(ms_delay) == 0) {
		return;
	}
	if (rtt_min == 0 || ms_delay < rtt_min)
		rtt_min = ms_delay;
	if (rtt_max == 0 || ms_delay > rtt_max)
		rtt_max = ms_delay;
	rtt_counter++;
	//rtt_avg = (rtt_avg*(avg_counter-1)/avg_counter)+(ms_delay/avg_counter);
	mpf_t ms_delay_mpf;
	mpf_init_set_d(ms_delay_mpf, ms_delay);
	mpf_add(rtt_sum, rtt_sum, ms_delay_mpf);

	mpf_t ms_delay_sq_mpf;
	mpf_init(ms_delay_sq_mpf);
	mpf_pow_ui(ms_delay_sq_mpf, ms_delay_mpf, 2);
	mpf_add(rtt_sumsq, rtt_sumsq, ms_delay_sq_mpf);
}

int minavgmax_jitter_history(float jitter_delay) {
	static int history_count = 0;
	static float jitter_avg_history = 0;

	float diff = jitter_avg_history * 8; // 800 percent
	float min_diff = (jitter_avg_history-diff);
	float max_diff = (jitter_avg_history+diff);

	if (history_count < 50 || (jitter_delay <= max_diff && jitter_delay >= min_diff)) { // Use 50 measures to create history (like RTP)
		if (history_count < 10000) { // Use maximum 10000 measures for history
				history_count++;
		}
		jitter_avg_history = (jitter_avg_history*(history_count-1)/history_count)+(jitter_delay/history_count);
		history_jitter_accepted++;
		return 1;
	} else {
		history_jitter_dropped++;
		return 0; //refuse delay
	}

}

void minavgmax_jitter(float ms_interarival_time, time_t curTime)
{
	//static int avg_counter = 0;
	if (check_pre_run_time(curTime) == 0) {
			return;
	}


	if (minavgmax_jitter_history(ms_interarival_time) == 0) {
		return;
	}
	if (jitter_min == 0 || ms_interarival_time < jitter_min)
		jitter_min = ms_interarival_time;
	if (jitter_max == 0 || ms_interarival_time > jitter_max)
		jitter_max = ms_interarival_time;
	jitter_counter++;
	mpf_t jitter_delay_mpf;
	mpf_init_set_d(jitter_delay_mpf, ms_interarival_time);
	mpf_add(jitter_sum, jitter_sum, jitter_delay_mpf);

	mpf_t jitter_delay_sq_mpf;
	mpf_init(jitter_delay_sq_mpf);
	mpf_pow_ui(jitter_delay_sq_mpf, jitter_delay_mpf, 2);
	mpf_add(jitter_sumsq, jitter_sumsq, jitter_delay_sq_mpf);
}


static inline void tvsub(struct timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

int rtt(int *seqp, int recvport, float *ms_delay)
{
	static time_t lastTime_sec = 0;
	static time_t lastTime_usec = 0;
	long sec_delay = 0, usec_delay = 0;
	long sec_diff = 0, usec_diff = 0;
	int i, tablepos = -1, status;

	if (*seqp != 0) {
		for (i = 0; i < TABLESIZE; i++)
			if (delaytable[i].seq == *seqp) {
				tablepos = i;
				break;
			}
	} else {
		for (i=0; i<TABLESIZE; i++)
			if (delaytable[i].src == recvport) {
				tablepos = i;
				break;
			}
			if (i != TABLESIZE)
				*seqp = delaytable[i].seq;
	}

	if (tablepos != -1)
	{
		status = delaytable[tablepos].status;
		delaytable[tablepos].status = S_RECV;


		if (lastTime_sec != 0 && lastTime_usec != 0) {
			time_t curTime_sec = time(NULL);
			time_t curTime_usec = get_usec();
			sec_diff = curTime_sec - lastTime_sec;
			usec_diff = curTime_usec - lastTime_usec;

			if (sec_diff == 0 && usec_diff < 0)
				usec_diff += 1000000;

			float ms_interarrival_time = (sec_diff * 1000) + ((float)usec_diff / 1000);
			minavgmax_jitter(ms_interarrival_time, curTime_sec);
			lastTime_sec = curTime_sec;
			lastTime_usec = curTime_usec;

		} else {
			lastTime_sec = time(NULL);
			lastTime_usec = get_usec();
		}
		time_t curTime = time(NULL);
		sec_delay = curTime - delaytable[tablepos].sec;
		usec_delay = get_usec() - delaytable[tablepos].usec;
		if (sec_delay == 0 && usec_delay < 0)
			usec_delay += 1000000;

		*ms_delay = (sec_delay * 1000) + ((float)usec_delay / 1000);

		minavgmax(*ms_delay, curTime);


	}
	else
	{
		*ms_delay = 0;	/* not in table.. */
		status = 0;	/* we don't know if it's DUP */
	}

	/* SANITY CHECK */
	if (*ms_delay < 0)
	{
		printf("\n\nSANITY CHECK in rtt.c FAILED!\n");
		printf("- seqnum = %d\n", *seqp);
		printf("- status = %d\n", status);
		printf("- get_usec() = %ld\n", (long int) get_usec());
		printf("- delaytable.usec = %ld\n", 
                       (long int) delaytable[tablepos].usec);
		printf("- usec_delay = %ld\n", usec_delay);
		printf("- time(NULL) = %ld\n", (long int) time(NULL));
		printf("- delaytable.sec = %ld\n", 
                       (long int) delaytable[tablepos].sec);
		printf("- sec_delay = %ld\n", sec_delay);
		printf("- ms_delay = %f\n", *ms_delay);
		printf("END SANITY CHECK REPORT\n\n");
	}

	return status;
}

void delaytable_add(int seq, int src, time_t sec, time_t usec, int status)
{
	delaytable[delaytable_index % TABLESIZE].seq = seq;
	delaytable[delaytable_index % TABLESIZE].src = src;
	delaytable[delaytable_index % TABLESIZE].sec = sec;
	delaytable[delaytable_index % TABLESIZE].usec = usec;
	delaytable[delaytable_index % TABLESIZE].status = status;
	delaytable_index++;
}

