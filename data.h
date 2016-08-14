/*
 * Data Processor Simulator, Sung Chul Lee
 */
#ifndef _DataProcessor
#define _DataProcessor

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <jni.h>
#include <string.h>

class DataProcessor {
	JNIEnv *env;
	jobject obj;
	static int count;
	int idn;
public:
	struct ExternalSignal {
		unsigned int dprr;

		struct BufferedSignal {
			unsigned int value,*ptr;
			
			unsigned int operator*(void) {
				return value;
			}
			void operator()(void) {
				value=(*ptr);
			}
			void operator=(unsigned int *ptr) {
				this->ptr=ptr;
			}
		} 	dpcr,
			ccd,
			eot,
			dpc,
			clr_irq;
	} signal;

private:
	struct ProcessorStatus {
		unsigned int done, result;
	} status;

	struct InternalRegister {
		unsigned int dprr;
	} reg;

public:
	DataProcessor(jobject obj);
	void Run();
	void setJNIEnv(JNIEnv *env);

private:
	void CaptureSignals();
	void ProcessRequest();
	void SendResult();
	void WriteSignals();
};

#endif
