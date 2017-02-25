#include "arduino_communication.h"

static char zz[]="zzzzzzz";

static char str_code[] = {'0','1','2','3','4','5','6','7','8','9','a','b','g','l','p'};
static RTC_DS1307 RTC;
static void PronounceNumber(uint8_t, char*);

/*--------------------------------------------------------------*/
void ArdCommOpen()
{
	Serial1.begin(115200);
	// Wire.begin();
	// RTC.begin();
}

uint8_t ArdCommWrite(uint8_t typeOfMessage, char* msg)
{
	static char str[10];
	static uint16_t timeHoldForSendVoiceMsg=0;
	static uint32_t lastMsgMillis;

	if (IsTimeExpired(lastMsgMillis, timeHoldForSendVoiceMsg) || typeOfMessage==SYSTEM)
	{
		sprintf(str, "%s", msg);
		strncat(str, zz, 8-strlen(str));	
		Serial1.println(str);
		// Serial.println(str);

		if (typeOfMessage==VOICE_DIRECTION)
		{
			timeHoldForSendVoiceMsg=1000;
		}
		else if (typeOfMessage==VOICE_TIME)
		{
			timeHoldForSendVoiceMsg=5000;
		}
		if (typeOfMessage!=SYSTEM)
		{
			StartCountTime(&lastMsgMillis);
		}
		return 1;
	}
	return 0;
}

uint8_t ArdCommWriteTime()
{
	DateTime now = RTC.now();
  char str[10], str_hour[5], str_min[5];
  PronounceNumber(now.hour(), str_hour);
  PronounceNumber(now.minute(), str_min);
  sprintf(str, "v%sg%s", str_hour, str_min);
  // ArdCommWrite(VOICE, (char*)str);
  // return ArdCommWrite(VOICE_TIME, (char*)"In Gio");
  return ArdCommWrite(VOICE_TIME, (char*)str);
}

uint8_t ArdCommWriteVol()
{
	return ArdCommWrite(SYSTEM, (char*)"p+");
}
/*--------------------------------------------------------------*/

void PronounceNumber(uint8_t n, char* out)
{	
	uint8_t chuc, dv;
	uint8_t pos = 0;

	chuc = n / 10;
	dv = n % 10;

	if (chuc==1)
	{
		out[pos++] = str_code[10];		
	}
	else if (chuc>1)
	{
		out[pos++] = str_code[chuc];
		out[pos++] = str_code[11];
	}

	if (dv!=0 || (chuc==0 && dv==0))
	{
		if (dv==5 && chuc!=0)
		{	
			out[pos++] = str_code[13];
		}
		else {
			out[pos++] = str_code[dv];
		}
	}
	out[pos] = '\0';
}
/*--------------------------------------------------------------*/