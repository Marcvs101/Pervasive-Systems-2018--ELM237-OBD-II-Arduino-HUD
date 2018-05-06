#pragma once

//ELM327 INTERFACE HEX VALUES FOR QUERIES
//Refer to https://cdn.sparkfun.com/assets/c/8/e/3/4/521fade6757b7fd2768b4574.pdf

//Standard codes
#define ELM_READY		">"
#define ELM_RESET		"AT Z"
#define ELM_NORMAL_MODE	"AT AT1"
#define ELM_FAST_MODE	"AT AT2"	//More aggressive