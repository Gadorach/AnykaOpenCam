/* SPDX-License-Identifier: GPL-2.0-only
 * Recorded Annex-B media -> shared RTP byte fixture. Does not execute firmware,
 * open hardware, create a network listener, or claim live video qualification.
 */
#include <camera/rtp_h264.h>
#include <stdio.h>
#include <stdlib.h>
static size_t start_code(const uint8_t *p,size_t bytes,size_t at,size_t *prefix)
{
 for(size_t i=at;i+3<=bytes;i++){
  if(!p[i]&&!p[i+1]){
   if(p[i+2]==1){*prefix=3;return i;}
   if(i+4<=bytes&&!p[i+2]&&p[i+3]==1){*prefix=4;return i;}
  }
 }
 *prefix=0;return bytes;
}
static int write_packet(void *context,const uint8_t *p,size_t bytes)
{return fwrite(p,1,bytes,context)==bytes?0:-1;}
int main(int argc,char **argv)
{
 if(argc!=4)return 2;
 char *end;unsigned long channel=strtoul(argv[3],&end,10);if(!argv[3][0]||*end||channel>254)return 2;
 FILE *input=fopen(argv[1],"rb");if(!input)return 1;
 uint8_t *data=malloc(1048577);if(!data){fclose(input);return 1;}
 size_t bytes=fread(data,1,1048577,input);int error=ferror(input);fclose(input);
 if(error||!bytes||bytes>1048576){free(data);return 1;}
 FILE *output=fopen(argv[2],"wbx");if(!output){free(data);return 1;}
 struct camera_rtp_session s={.ssrc=0x51430000|(uint32_t)channel,.sequence=65535,.channel=(uint8_t)channel,.payload_type=96};
 size_t prefix,at=start_code(data,bytes,0,&prefix);int result=1;
 if(at)goto done;
 while(prefix){
  size_t next_prefix,nal=at+prefix,next=start_code(data,bytes,nal,&next_prefix);
  if(camera_rtp_h264_nal(&s,90000,data+nal,next-nal,next==bytes,write_packet,output)!=CAMERA_OK)goto done;
  at=next;prefix=next_prefix;
 }
 result=0;
done:
 if(fclose(output))result=1;free(data);return result;
}
