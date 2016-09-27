#**miniupnpd**  
##下载源码  
    git clone https://github.com/miniupnp/miniupnp.git  
##修改源码  
###1.miniupnpd.c的修改，设备网页（presentationurl）的初始化修改.未修改前：
	if(presurl)
	{
		strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
		presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';
	}
	else
	{   
		snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
		         "http://%s/",lan_addrs.lh_first->str);
		         /*"http://%s:%d/", lan_addrs.lh_first->str, 80);*/
	} 
###修改后：
	if(presurl)
	{
		strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
		//presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';
	}
	else
	{   strcpy(presentationurl, "/");
		//snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
		     //    "http://%s/",lan_addrs.lh_first->str);
		         /*"http://%s:%d/", lan_addrs.lh_first->str, 80);*/
	}  
###2.minissdp.c的修改  
####SendSSDPResponse方法的修改，增加对不同网卡ip地址的赋值，修改前：  
	if(l<0)
	{
		syslog(LOG_ERR, "%s: snprintf failed %m",
		       "SendSSDPResponse()");
		return;
	}
###修改后
	snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
		         "http://%s/", host);
	if(l<0)
	{
		syslog(LOG_ERR, "%s: snprintf failed %m",
		       "SendSSDPResponse()");
		return;
	}

####进一步对SendSSDPNotify方法的修改，更新，未修改前：  
	char bufr[SSDP_PACKET_MAX_LEN];
	int n, l;
	l = snprintf(bufr, sizeof(bufr),
		"NOTIFY * HTTP/1.1\r\n"
		"HOST: %s:%d\r\n"
		"CACHE-CONTROL: max-age=%u\r\n"
		"LOCATION: http://%s:%u" ROOTDESC_PATH "\r\n"
###修改后：
	snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
		        "http://%s/",host);
	char bufr[SSDP_PACKET_MAX_LEN];
	int n, l;
	l = snprintf(bufr, sizeof(bufr),
		"NOTIFY * HTTP/1.1\r\n"
		"HOST: %s:%d\r\n"
		"CACHE-CONTROL: max-age=%u\r\n"
		"LOCATION: http://%s:%u" ROOTDESC_PATH "\r\n" 
####3.对设备类型的定义修改，未修改前： 
	/* IGD v1 */
	#define DEVICE_TYPE_IGD     "urn:schemas-upnp-org:device:InternetGatewayDevice:1"
	#define DEVICE_TYPE_WAN     "urn:schemas-upnp-org:device:WANDevice:1"
	#define DEVICE_TYPE_WANC    "urn:schemas-upnp-org:device:WANConnectionDevice:1"
	#define SERVICE_TYPE_WANIPC "urn:schemas-upnp-org:service:WANIPConnection:1"
	#define SERVICE_ID_WANIPC   "urn:upnp-org:serviceId:WANIPConn1" 
###修改后： 
	#define DEVICE_TYPE_IGD     "urn:schemas-upnp-org:device:unknown:1"
	#define DEVICE_TYPE_WAN     "urn:schemas-upnp-org:device:unknown:1"
	#define DEVICE_TYPE_WANC    "urn:schemas-upnp-org:device:WANConnectionDevice:1"
	#define SERVICE_TYPE_WANIPC "urn:schemas-upnp-org:service:WANIPConnection:1"
	#define SERVICE_ID_WANIPC   "urn:upnp-org:serviceId:WANIPConn1" 
####6.对miniupnpd.c修改，未修改前： 
	if(!ext_if_name || !lan_addrs.lh_first)
	{
		/* bad configuration */
	
		goto print_usage;
	} 
#####修改后：  
	if(!ext_if_name || !lan_addrs.lh_first)
	{
		/* bad configuration */
		printf("1-------------ext_if_name%s\n",ext_if_name );
		printf("2-------------lan_addrs.lh_first%s\n", lan_addrs.lh_first);
		//goto print_usage;
	}
	
####5.其它的修改是进行一些测试的代码

##配置交叉编译环境  
####进入交叉编译工具的目录，用source命令配置环境 

    source environment-setup-i586-wrs-linux   
##编译命令  
###IPTABLESPATH 赋值为iptables-1.4.21路径，后面加make -f Makefile.linux命令
    IPTABLESPATH=/home/ssf/miniupnp/miniupnpd/iptables-1.4.21/ make -f Makefile.linux  
##复制到开发板  
	scp -r bin/ conf/ root@192.168.1.30:/opt/test

##ssh进入开发板
    ssh root@192.168.1.30  
##运行  
###主要使用 -a 命令配置网卡
	bin/miniupnpd -d -a eth0 -a eth1
##配置文件miniupnpd.conf主要信息
	listening_ip=eth0    配置监听网卡
	listening_ip=eth1    配置监听网卡
	friendly_name=XinGuoUPnP     设备名字
	manufacturer_name=XinGou     制造商名字  
	manufacturer_url=http://192.168.1.101/   制造商网址  
	model_name=Xin Guo  型号名字  
	model_url=http://192.168.1.33/    型号网页  
	serial=12345678    序列号
	model_number=1     版本号