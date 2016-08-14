#include "data.h"
#include <pthread.h>

int DataProcessor::count=0;

DataProcessor::DataProcessor(jobject obj) {
	this->obj=obj;
	idn=count++;
	memset(&signal,0,sizeof(signal));
	memset(&status,0,sizeof(status));
	memset(&reg,0,sizeof(reg));
}

void DataProcessor::setJNIEnv(JNIEnv *env){
	this->env=env;
}



void DataProcessor::Run() {

	CaptureSignals();

	if(status.done==0)
		ProcessRequest();

	else if(status.done==1)
		SendResult();

	WriteSignals();
}

void DataProcessor::ProcessRequest() {
	if(*signal.dpc == 0)
		return;
	printf("Running data call id=%d\n", idn);
	unsigned int id,
		ccd		 = (0xF & (*signal.ccd) ),
		case_num = (0xFF & (*signal.dpcr) );

	switch(ccd)	{
		case 1: id=0; break;
		case 2: id=1; break;
		default:id=0; break;
	}
	char func[128];
	sprintf(func,"cbackcall%d",id);

	jclass 		cls	= (env)->GetObjectClass(obj);
	jmethodID	mid	= (env)->GetMethodID(cls,func,"(I)Z");
	if(mid == NULL) {
		printf("method %s not found\n", func);
		exit(1);
	}
	printf("> data processor call\n");
	printf("    current clock domain %d(%d)\n", id, ccd);
	printf("    case number %d\n", case_num);
	status.result = (env)->CallBooleanMethod(obj,mid,case_num);
	status.done = 1;
}

void DataProcessor::SendResult() {
	if(*signal.clr_irq) {
		status.done=0;
		status.result=0;
		printf("interrupt cleared\n");
	}
	reg.dprr=(status.done << 1) + (status.result & 1);
}

extern pthread_mutex_t mutex1;
extern pthread_mutex_t mutex2;

void DataProcessor::CaptureSignals() {
	//printf("DataProcessor::CaptureSignals()\n");
	pthread_mutex_lock(&mutex1);
	signal.dpcr();
	signal.ccd();
	signal.eot();
	signal.dpc();
	signal.clr_irq();
	pthread_mutex_unlock(&mutex1);
}

void DataProcessor::WriteSignals() {
	//printf("DataProcessor::WriteSignals()\n");
	pthread_mutex_lock(&mutex2);
	signal.dprr=reg.dprr;
	pthread_mutex_unlock(&mutex2);
}
