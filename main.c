#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define CRYPTOCOMPARE_SINGLE_PRICE "https://min-api.cryptocompare.com/data/price"
#define CRYPTOCOMPARE_MULTI_PRICE "https://min-api.cryptocompare.com/data/pricemulti"

struct ResponseData {
				char* htmlData;
};

float getCurrencyPrice(char* tickers[], short tickerCount);
size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata);
char* getJSONLevel(char* jsonString, char* key);
char* cleanupJSONTier(char* tier);
char* retrieveCurrentPrice(char* data, char* ticker);

int main(int argc, char** argv){
				char* tickers[] = {"ETH", "NANO", "LTC", "BTC", "XMR", "XRB", "ADA"};
				float ethPrice = getCurrencyPrice(tickers, 7);
				return 0;
}

float getCurrencyPrice(char* tickers[], short tickerCount){
				CURL* curl;
				CURLcode res;

				curl = curl_easy_init();
				if(!curl){
								printf("Curl could not be initialized!\n");
								return 1;
				}
				struct ResponseData responseData;
				if(tickerCount == 1){
								curl_easy_setopt(curl, CURLOPT_URL, "https://min-api.cryptocompare.com/data/price?fsym=ETH&tsyms=USD");
				}else{
								// Calculate the total length of the ticker string.
								int stringLen = 0;
								for(int c=0;c!=tickerCount;c++){
												char* currentTicker = tickers[c];
												stringLen += strlen(currentTicker);
								}
								char* currencyList = (char*)malloc(stringLen + tickerCount);
								int listPosition = 0;
								for(int c=0;c!=tickerCount;c++){
												memcpy(currencyList + listPosition, tickers[c], strlen(tickers[c]));
												currencyList[strlen(tickers[c])+listPosition] = ',';
												listPosition += strlen(tickers[c])+1;
								}
								currencyList[stringLen + tickerCount] = 0;
								// Now generate the request URL for libcurl.
								int requestURLLength = strlen(CRYPTOCOMPARE_MULTI_PRICE) + strlen("?fsyms=") + strlen(currencyList) + strlen("&tsyms=USD\0");
								char* requestURL = (char*)malloc(sizeof(char) * requestURLLength);
								strcpy(requestURL, CRYPTOCOMPARE_MULTI_PRICE);
								strcat(requestURL, "?fsyms=");
								strcat(requestURL, currencyList);
								strcat(requestURL, "&tsyms=USD\0");
								curl_easy_setopt(curl, CURLOPT_URL, requestURL);
				}
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleRemoteResponse);
				res = curl_easy_perform(curl);
				// We now have the response, parse it.
				char* currentPrice;
				for(int c=0;c!=tickerCount;c++){
								currentPrice = retrieveCurrentPrice(responseData.htmlData, tickers[c]);
								printf("%s: %s\n", tickers[c], currentPrice);
				}
}


size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata){
				struct ResponseData* thisResponse = (struct ResponseData*)userdata;
				thisResponse->htmlData = (char*)malloc(size*nmemb) + sizeof(char);
				memcpy(thisResponse->htmlData, ptr, size*nmemb);
				thisResponse->htmlData[size*nmemb] = 0;
				return size*nmemb;
}

// The following is a barely function pseudo-tokenizer monstrosity that is here
// for the sole purpose of avoiding linking a full JSON parsing library.
// It works just well enough to get the job done.  I don't expect any awards.
char* getJSONLevel(char* jsonString, char* key){
				// Scan through the provided JSON string until the proper key is found.
				char* keyLocation = strstr(jsonString, key);
				// We now have the key location, locate the end of it and return this
				// value.
				char* needle = (char*)malloc(sizeof(char)*2);
				needle = ":\0";
				char* returnVal = strstr(keyLocation, needle);
				return returnVal;
}

// Truncate at the first occurence of '}'
char* cleanupJSONTier(char* tier){
				tier += 1;
				char* needle = (char*)malloc(sizeof(char)*1);
				needle = "}";
				char* breakPos = strstr(tier, needle);
				char* returnString = (char*)malloc(sizeof(char)*(breakPos-tier));
				memcpy(returnString, tier, (int)(breakPos-tier));
				returnString[breakPos-tier] = 0;
				return returnString;
};

char* retrieveCurrentPrice(char* data, char* ticker){
				char* thisStart = getJSONLevel(data, ticker);				
				char* usd = getJSONLevel(thisStart, "USD");
				usd = cleanupJSONTier(usd);
				return usd;
}
