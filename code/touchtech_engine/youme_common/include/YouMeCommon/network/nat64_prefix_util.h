#ifndef SOCKET_NAT64_PREFIX_UTIL_H_
#define SOCKET_NAT64_PREFIX_UTIL_H_

/*
 * WARNING:All functions below may be blocked when called first time, please don't use these functions in main thread.
 * if current network is not ipv6-only, these fuction all will return false
 * */
#include <string>





#ifdef __APPLE__
#ifndef s6_addr16
#define	s6_addr16   __u6_addr.__u6_addr16
#endif

#ifndef s6_addr32
#define	s6_addr32   __u6_addr.__u6_addr32
#endif
#endif


/*
	* param: _nat64_prefix, return the nat64 prefix, using a string
	* return: if return false, then _nat64_prfix is empty string.
	* */
bool  GetNetworkNat64Prefix(std::string& _nat64_prefix);

/*
	* param: _nat64_prefix_in6, return the nat64 prefix, using struct in6_addr.
	* 		  _nat64_prefix_in6.s6_addr32[0~2](12 Bytes) contain the nat64 prefix
	* return: if return false, _nat64_prefix_in6 will not change.
	* */
bool  GetNetworkNat64Prefix(struct in6_addr& _nat64_prefix_in6);


/*
	* param: _v4_ip:the input v4 ip, _nat64_v6_ip the output v6 ip, which embeded _v4_ip with format RFC6052
	* return: if return false(MAY BE INVALID _v4_ip), _nat64_v6_ip will not change.
	* */
bool ConvertV4toNat64V6(const std::string& _v4_ip, std::string& _nat64_v6_ip);

/*
	* param: _v4_addr input v4 addr, _v6_addr the output v6 addr, which embeded _v4_addr with format RFC6052
	* return: if return false, _v6_addr will not change.
	* */
bool ConvertV4toNat64V6(const struct in_addr& _v4_addr, struct in6_addr& _v6_addr);


#endif /* SOCKET_NAT64_PREFIX_UTIL_H_ */
