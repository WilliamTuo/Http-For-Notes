
#include <vector>

#define WEBROOT "./wwwroot"
#define HOMEPAGE "index.html"
//#define HOMEPAGE "test.html"

std::vector<std::string> RequestMethod = {	"GET", "HEAD", "POST", "PUT"		
										, "DELETE", "CONNECT", "OPTIONS"
										, "TRACE"	, "PATCH", "MOVE"	
										, "COPY"	, "LINK"	, "UNLINK"	
										, "WRAPPED"
										};


