#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CRYPTOCOMPARE_SINGLE_PRICE "https://min-api.cryptocompare.com/data/price"
#define CRYPTOCOMPARE_MULTI_PRICE "https://min-api.cryptocompare.com/data/pricemulti"

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[37;0m"

struct ResponseData {
				char* htmlData;
};

struct CLIArgs{
				short flags;
				short currencyCount;
				char** currencies;
};

float getCurrencyPrice(char* tickers[], short tickerCount);
size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata);
char* getJSONLevel(char* jsonString, char* key);
char* cleanupJSONTier(char* tier);
char* retrieveCurrentPrice(char* data, char* ticker);
struct CLIArgs* doArgs(int argc, char** argv);

int main(int argc, char** argv){
				struct CLIArgs* cliArgs = doArgs(argc, argv);
				float ethPrice = getCurrencyPrice(cliArgs->currencies, cliArgs->currencyCount);
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
								sprintf(requestURL, "%s?fsyms=%s&tsyms=USD\0", CRYPTOCOMPARE_MULTI_PRICE, currencyList);
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

struct CLIArgs* doArgs(int argc, char** argv){
				// Parse the flags given to the software on the commandline.
				struct CLIArgs* returnArgs = (struct CLIArgs*)malloc(sizeof(struct CLIArgs));
				if(argc == 1){ // No arguments were specified.
								return NULL;
				}
				// Arguments have been specified.  Process them.
				for(int c=1;c!=argc;c++){
								char* needle = (char*)malloc(sizeof(char)*2);
								needle = "-f";
								if(strstr(argv[c], needle)){
												// The currency flag has been specified.  Parse out the
												// list and assign it to the struct.
												char* listStart = argv[c+1];
												// We should now be at the list start.
												// Determine the number of currencies in the list based
												// on the number of commas within the string.
												// Generate a 2-dimensional array containing all of the
												// specified currencies.
												short numCurrencies = 0;
												for(short x=0;x!=strlen(listStart);x++){
																if(listStart[x] == ','){
																				numCurrencies++;
																}
												}
												numCurrencies++;
												returnArgs->currencyCount = numCurrencies;
												// We now have the total number of currencies, allocate
												// space for them.
												returnArgs->currencies = (char**)malloc(sizeof(char*)*numCurrencies);
												// The loop for each character in the list.
												short rawTickerLen = 0;
												short currentCount = 0;
												char* tickerPtr = listStart;
												for(short lc=0;lc!=strlen(listStart)+1;lc++){
																// Keep counting until either EOL or the first comma.
																if(listStart[lc] != ',' && listStart[lc] != 0){
																				rawTickerLen++;
																}else{
																				// Determine the number of spaces.
																				short numNonSpaces = 0;
																				for(short v=0;v!=rawTickerLen;v++){
																								if(tickerPtr[v] != ' '){
																												numNonSpaces++;
																								}
																				}
																				char* thisString = (char*)malloc(sizeof(char)*numNonSpaces);
																				short currentPos = 0;
																				for(short v=0;v!=rawTickerLen;v++){
																								if(tickerPtr[v] != ' '){
																												thisString[currentPos] = tickerPtr[v];
																												currentPos++;
																								}
																				}
																				// Convert the letters to uppercase for
																				// particularly picky APIs
																				for(short v=0;v!=numNonSpaces;v++){
																								thisString[v] = toupper(thisString[v]);
																				}	
																				tickerPtr = &listStart[lc]+1;
																				rawTickerLen = 0;
																				returnArgs->currencies[currentCount] = thisString;
																				currentCount++;
																}
												}
								}
				}
				return returnArgs;
}
