/*
 * Copyright (c) 2012 Hanspeter Portner (agenthp@users.sf.net)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#include <stdlib.h>
#include <string.h>

#include <ipv4ll.h>

const uint8_t link_local_gateway [] = {169, 254, 0, 0};
const uint8_t link_local_subnet [] = {255, 255, 0, 0};

static void 
IPv4LL_random (uint8_t *ip)
{
	ip[0] = 169;
	ip[1] = 254;
	ip[2] = 1 + rand () / (RAND_MAX / 253);
	ip[3] = rand () / (RAND_MAX / 255);
}

void
IPv4LL_claim (uint8_t *ip, uint8_t *gateway, uint8_t *subnet)
{
	uint8_t link_local_ip [4];

	do {
		IPv4LL_random (link_local_ip);
	} while (arp_probe (0, link_local_ip)); // collision?

	arp_announce (0, link_local_ip);

	memcpy (ip, link_local_ip, 4);
	memcpy (gateway, link_local_gateway, 4);
	memcpy (subnet, link_local_subnet, 4);
}