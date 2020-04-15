//Declare Including Header/Library Files
#include <types.h>
#include <reg.h>
#include <serial.h>
#include <lib.h>
#include <time.h>
#include "Image_Data.h"
//Declare Global Pointer Varaibles
staticunsignedshort* cam_addr;
volatileunsignedshort* lcd_addr;
volatileunsignedshort* modeset;
unsignedchar* led_addr;
//LED ADDRESS
unsignedchar* buzzer_addr;
#define ADDR_OF_LED		0x14805000
//Keypad Address Define
#define ADDR_OF_KEY_LD	0x14806000
#define ADDR_OF_KEY_RD	0x14807000
//CLCD ADDRESS DEFINE
#define ADDR_OF_CLCD		0x14809000
unsignedshort* Clcd_addr;
//Camera/LCD Address Define
#define CIS_1300K_SRAM_ADDRESS		0x16900000
#define CIS_1300K_IRQ_ENABLE		0x16980060
#define CIS_1300K_MODE_ADDRESS		0x16980066
#define FLB_ADD	0x80600400
#define FLB(x)	(*(volatile unsigned long*)(FLB_ADD + x))/* Frame Buffer */
//Buzzer Setting
#define ADDR_OF_Buzzer	0x14808000
//Define Camera/LCD Constants
#define CIS_IMAGE_WIDTH	320
#define CIS_IMAGE_HEIGHT	240
#define CIS_VAL_SET		0x00
//Define Global Variable
staticunsignedint read_st = 0;
/* 1.3M CMOS Camera Test[FPGA Path]  */
///Define State Variable Values
//Program States
#define DEBUG_CAMERA				999
#define MAIN_MENU					000
#define HOW_TO_PLAY					100
#define SELECT_DIFFICULTY				101

#define GAME_START					200
#define CAMERA_SETTING_START			201
#define CAMERA_SETTING_ENDED			202
#define ROUND_NUM_DISPLAY				210
#define COUNTDOWN_3					211
#define COUNTDOWN_2					212
#define COUNTDOWN_1					213

#define CAPTURE_IMAGE				300
#define COMPARE					301

#define DISPLAY_FAIL					400
#define DISPLAY_WIN					401
#define DISPLAY_ALL_WIN				402

#define IDLE				0
#define ACTIVE				1

//State Constants & Variables
unsignedshort difficulty = 0;
#define EASY 1
#define HARD 2
int difficulty_point_weight = 0;
#define EASY_WEIGHT 10
#define HARD_WEIGHT 15
int Point_Sum = 0;
#define POINT_RAND 1

///Define Game CONSTANTS
//ERROR THRESHOLD VALUES
float ERROR_THRESHOLD = 0.0;
float PERCENT_THRESHOLD = 0.0;
#define ERROR_THRESHOLD_EASY 30.0 	//Integer(Max 360)		//50
#define PERCENT_THRESHOLD_EASY 10.0	//Percentage(Max 100%)
#define ERROR_THRESHOLD_HARD 15.0
#define PERCENT_THRESHOLD_HARD 5.0
//Error Check Variables & Constants
unsignedint Error_Count = 0;
unsignedint Error_Pixel_Count = 0;
float Error_Percentage = 0;
float R_saved, G_saved, B_saved, R_captured, G_captured, B_captured;
float H_saved, H_captured, max_saved, max_captured, min_saved, min_captured, dt_saved, dt_captured;
//DELAY CONSTANTS & Variables
unsignedlong delay_count = 0;
unsignedlong delay_sub_count = 0;
#define DELAY_SAVING 8
#define DELAY_SAVED 2
#define DELAY_ROUND_STR 4
#define DELAY_ROUND_NUM 2
unsignedlong DELAY_COUNTDOWN = 0;
#define DELAY_COUNTDOWN_EASY 4
#define DELAY_COUNTDOWN_HARD 2
#define DELAY_DEBUG 10
#define DELAY_COUNTDOWN_PREDELAY 1//Don't Change
#define DELAY_ERROR_IMAGE_DISPLAY 3500
#define DELAY_GAME_END 8
//Buzzer Control Varaibles & Constants
#define DELAY_BUZZER_SHORT 80
#define DELAY_BUZZER_LONG 250
unsignedint stc_length = 0;
unsignedint stc_sub_length = 0;
//GAMEPLAY_CONSTANTS
unsignedshort round_num = 0;
//Data Buffer
unsignedshort Image_Saved[CIS_IMAGE_WIDTH][CIS_IMAGE_HEIGHT];
unsignedshort Image_Capture[CIS_IMAGE_WIDTH][CIS_IMAGE_HEIGHT];
unsignedshort Image_Temp[CIS_IMAGE_WIDTH][CIS_IMAGE_HEIGHT];
float Image_Error[CIS_IMAGE_WIDTH][CIS_IMAGE_HEIGHT];
#define MAXIMUM_ROUND 3
//Color Setting
#define RED			0xF800
#define BLUE		0x1F
#define GREEN		7E0
#define VIOLET	0x4010
//Define System Setting
int i, j, k;
int flag;
char ch;
//Declare SubFunction Prototypes
unsignedchar key_scan(unsignedchar* key_addr, unsignedchar* key_addr_rd);	//Keypad Scan Function
void Display_Main_Menu(void);		//Show Main Menu Graphic on TFT-LCD Function
void Display_How_To_Play(void);	//Show How to Play Graphic on TFT-LCD Function
void Display_Select_Difficulty(void);	//Show Select Difficulty Graphic on TFT-LCD Function
void Ring_Buzzer_Short(void);	//Ring Buzzer For Short Time
void Ring_Buzzer_Long(void);	//Ring Buzzer For Long Time
//Character LCD Functions
void setcommand(unsignedshort command);
void writebyte(char ch);
void initialize_textlcd();
void write_string(char* str, int length);
int function_set(int rows, int nfonts);
int display_control(int display_enable, int cursor_enable, int nblink);
int cusrsor_shit(int set_screen, int set_rightshit);
int entry_mode_set(int increase, int nshift);
int return_home();
int clear_display();
int set_ddram_address(int pos);
void textlcd_score(int round, int score);	//Display Round/Score Data to Character LCD and LED
//Calculate Functions
#define ABS(X) X>0? X:-X
#define MAX(X,Y) X>Y? X:Y
#define MIN(X,Y) X<Y? X:Y
//Main Function Start
void c_main(void)
{
	////////////////////Initialize///////////////////////
	//Declare Local Variables
	//Camera/LCD Variables
	cam_addr = (unsignedshort*)CIS_1300K_SRAM_ADDRESS;
	lcd_addr = (unsignedshort*)FLB_ADD;
	modeset = (unsignedshort*)CIS_1300K_MODE_ADDRESS;
	led_addr = (unsignedchar*)ADDR_OF_LED;
	*buzzer_addr = 0x00;
	*modeset = CIS_VAL_SET;
	//Keypad Variables       
	unsignedchar* key_addr;
	unsignedchar* key_addr_rd;
	unsignedchar key_value = 0;
	unsignedchar old = 0;
	//Initialize Variables
	unsignedshort state = MAIN_MENU;	//Game Start	->	Main Menu
	unsignedshort idle_flag = ACTIVE;		//						Show Menu Graphic
	int x = 0;
	///System Initialize
	buzzer_addr = (unsignedchar*)ADDR_OF_Buzzer;
	*(unsignedint*)GPIO58_MFPR = 0xC800;
	__REG(GPIO_BASE + GPDR1) = __REG(GPIO_BASE + GPDR1) | ~(1 << 26);  //GP58, P320
	__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;
	key_addr = (unsignedchar*)ADDR_OF_KEY_LD;
	key_addr_rd = (unsignedchar*)ADDR_OF_KEY_RD;
	while (1) {
		//State=MAIN_MENU || HOW_TO_PLAY
		//Gameplay? 
		if (state == MAIN_MENU || state == HOW_TO_PLAY || state == DEBUG_CAMERA || state == SELECT_DIFFICULTY) {
			if (state == MAIN_MENU) {
				if (idle_flag == ACTIVE) {	//Show Menu Graphic on TFT-LCD and Keypad Menu on Terminal
					Display_Main_Menu();		//Display Graphic on TFT-LCD
					idle_flag = IDLE;
					printf("\n\n=====BODY MOVING GAME=====\n");		//Show Keypad Menu on Terminal
					printf("Choose Menu with Keypad:\n");
					printf("  1. How to Play\n");
					printf("  2. Game Start\n");
					printf("Press 'q' or 'Q' key to exit...\n");
				}
				key_value = key_scan(key_addr, key_addr_rd);//Keypad Scan
				if ((key_value != old) && (key_value > 0)) {
					if (key_value == 1) {
						Ring_Buzzer_Short();
						state = HOW_TO_PLAY;	//Next State->How to Play
						idle_flag = ACTIVE;
					}
					elseif(key_value == 2) {
						Ring_Buzzer_Short();
						state = SELECT_DIFFICULTY;		//Next State->Select Difficulty
						idle_flag = ACTIVE;
					}
					elseif(key_value == 3) {
						state = DEBUG_CAMERA;	//Next State->Debug Camera(Show Camera)
						idle_flag = ACTIVE;
					}
					msWait(250);	//Keypad Debouncing
				}
			}
			elseif(state == HOW_TO_PLAY) {
				if (idle_flag == ACTIVE) {	//Show How to Play Graphic on TFT-LCD and Keypad Menu on Terminal
					Display_How_To_Play();
					idle_flag = IDLE;
					printf("\n=====How To Play=====\n");
					printf("  1. Back to Main Menu\n");
				}
				key_value = key_scan(key_addr, key_addr_rd);
				if ((key_value != old) && (key_value > 0)) {
					if (key_value == 1) {
						Ring_Buzzer_Short();
						state = MAIN_MENU;		//Next State -> Main Menu
						idle_flag = ACTIVE;
					}
					msWait(250);	//Keypad Debouncing
				}
			}
			elseif(state == DEBUG_CAMERA) {	//Display Camera Debugging(Camera Position Setting Mode)
				if (idle_flag == ACTIVE) {
					idle_flag = IDLE;
					printf("\n=====DEBUG_CAMERA=====\n");
					printf("  1. Back to Main Menu\n");
					Ring_Buzzer_Long();
				}
				key_value = key_scan(key_addr, key_addr_rd);
				if ((key_value != old) && (key_value > 0)) {
					if (key_value == 1) {
						Ring_Buzzer_Short();
						state = MAIN_MENU;
						idle_flag = ACTIVE;
					}
					msWait(250);	//Keypad Debouncing
				}
				//Display Camera To TFT-LCD
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
						}
					}
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
				}
			}
			elseif(state == SELECT_DIFFICULTY) {	//Select Difficulty Mode
				if (idle_flag == ACTIVE) {	//Run First?
					Display_Select_Difficulty();	//Display Graphic on TFT-LCD
					idle_flag = IDLE;
					printf("\n\n=====Select Difficulty=====\n");
					printf("Choose Menu with Keypad:\n");
					printf("  1. Easy Mode\n");
					printf("  2. HARD Mode\n");
					printf("  3. Back To Main Menu");
					printf("Press 'q' or 'Q' key to exit...\n");
				}
				key_value = key_scan(key_addr, key_addr_rd);	//Keypad Scan==0?No Change:State Change
				if ((key_value != old) && (key_value > 0)) {
					if (key_value == 1) {
						Ring_Buzzer_Short();
						state = GAME_START;
						idle_flag = ACTIVE;
						difficulty = EASY;
						printf("EASY Mode Selected!\n");
						printf("\t:Slow Countdown, Less Point!\n\n");
					}
					elseif(key_value == 2) {
						Ring_Buzzer_Short();
						state = GAME_START;
						difficulty = HARD;
						idle_flag = ACTIVE;
						printf("HARD Mode Selected!\n");
						printf("\t:Fast Countdown, More Point!\n\n");
					}
					elseif(key_value == 3) {
						printf("Go Back to Main Menu!\n\n");
						Ring_Buzzer_Short();
						state = MAIN_MENU;
						idle_flag = ACTIVE;
					}
					msWait(250);
				}
			}
		}
		else {//State=GAME START~~
			switch (state) {
			case GAME_START:
				//Initialize all Game Varaibles
				printf("Game START!\n");
				Ring_Buzzer_Short();
				Ring_Buzzer_Short();
				//Initialize Game Variables
				round_num = 1;
				delay_count = 0;
				stc_length = 0;
				stc_sub_length = 0;
				Point_Sum = 0;
				textlcd_score(round_num, (int)Point_Sum);
				state = CAMERA_SETTING_START;
				if (difficulty == EASY) {
					DELAY_COUNTDOWN = DELAY_COUNTDOWN_EASY;
					difficulty_point_weight = EASY_WEIGHT;
					ERROR_THRESHOLD = ERROR_THRESHOLD_EASY;
					PERCENT_THRESHOLD = PERCENT_THRESHOLD_EASY;
				}
				elseif(difficulty == HARD) {
					DELAY_COUNTDOWN = DELAY_COUNTDOWN_HARD;
					difficulty_point_weight = HARD_WEIGHT;
					ERROR_THRESHOLD = ERROR_THRESHOLD_HARD;
					PERCENT_THRESHOLD = PERCENT_THRESHOLD_HARD;
				}
				//Print Setting Result to Terminal
				printf("Point Weight: %d\n", difficulty_point_weight);
				printf("Error Threshold : %d\n", (int)ERROR_THRESHOLD);
				printf("Error Percent Threshold : %d\n", (int)PERCENT_THRESHOLD);
				break;

			case CAMERA_SETTING_START:
				//Camera Setting Ready &  Aware User to Clear Camera View Sight
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							if (stc_length < stc_saving_length) {	//Camera Setting Message Display on TFT-LCD
								if (i == stc_saving[stc_length][0] && j == stc_saving[stc_length][1]) {
									*(lcd_addr + k) = (unsignedshort)RED;
									stc_length++;
								}
								else {
									*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
								}
							}
							else {
								*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
							}
						}
					}//Display_Image End
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
					stc_length = 0;
					//Delay Check
					delay_count++;
					if (delay_count > DELAY_SAVING) {	//Camera Image Capturing&Saving to Image_Saved
						Ring_Buzzer_Short();
						printf("Background Image Capturing......");
						while (idle_flag == ACTIVE) {
							flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
							if (flag == 0) read_st = 0;
							if (flag == 1 && read_st == 0) {
								__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
								read_st = 1;
								//Camera Image Save(Background)
								for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
									for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
										k = i * CIS_IMAGE_WIDTH + j;
										Image_Saved[j][i] = (unsignedshort)(*(cam_addr + k));	//Saved Image<-Camera Image
									}
								}
								__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
								idle_flag = IDLE;
							}
						}
						printf("Captured!\n");
						Ring_Buzzer_Long();
						state = CAMERA_SETTING_ENDED;
						idle_flag = ACTIVE;
						delay_count = 0;
					}
				}
				break;

			case CAMERA_SETTING_ENDED:
				//Display Message And Camera Image to TFT-LCD
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							if (stc_length < stc_saved_length) {
								//Camera Set Message Print Over Camera image of TFT-LCD
								if (i == stc_saved[stc_length][0] && j == stc_saved[stc_length][1]) {
									*(lcd_addr + k) = BLUE;
									stc_length++;
								}
								else {
									*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
								}
							}
							else {
								*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
							}
						}
					}
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
					stc_length = 0;
					stc_sub_length = 0;
					//Delay Check
					delay_count++;
					if (delay_count > DELAY_SAVED) {
						state = ROUND_NUM_DISPLAY;
						delay_count = 0;
						stc_length = 0;
						stc_sub_length = 0;
						idle_flag = ACTIVE;
						break;
					}
				}
				break;

			case ROUND_NUM_DISPLAY:
				//Print Round Message Over Camera Image
				if (idle_flag == ACTIVE) {
					switch (round_num) {
					case 1:
						Ring_Buzzer_Short();
						printf("Round: 1\n");
						break;
					case 2:
						Ring_Buzzer_Short();
						Ring_Buzzer_Short();
						printf("Round: 2\n");
						break;
					case 3:
						Ring_Buzzer_Short();
						Ring_Buzzer_Short();
						Ring_Buzzer_Short();
						printf("Round: 3\n");
						break;
					default:
						printf("ROUND_NUM_DISPLAY CASE:DEFAULT ERROR\n");
						return;
					}
					idle_flag = IDLE;
				}
				else {
					//Mode Select by Round Number
					switch (round_num) {
					case 1:
						flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
						if (flag == 0) read_st = 0;
						if (flag == 1 && read_st == 0) {
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
							read_st = 1;
							for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
								for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
									k = i * CIS_IMAGE_WIDTH + j;
									//Display Round Characters and Number on TFT-LCD
									if (stc_length < stc_round_length || stc_sub_length < stc_round_num_1_length) {
										if (i == stc_round[stc_length][0] && j == stc_round[stc_length][1]) {
											*(lcd_addr + k) = RED;
											stc_length++;
										}
										//Display Round Number On TFT-LCD
										elseif(delay_count > DELAY_ROUND_NUM&& i == stc_round_num_1[stc_sub_length][0] && j == stc_round_num_1[stc_sub_length][1]) {
											*(lcd_addr + k) = RED;
											stc_sub_length++;
										}
										else {
											*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
										}
									}
									else {
										*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
									}
								}
							}
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
							stc_length = 0;
							stc_sub_length = 0;
							//Delay Counting
							delay_count++;
						}
						break;
					case 2:
						flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
						if (flag == 0) read_st = 0;
						if (flag == 1 && read_st == 0) {
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
							read_st = 1;
							for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
								for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
									k = i * CIS_IMAGE_WIDTH + j;
									//Display Round Characters and Number On TFT-LCD
									if (stc_length < stc_round_length || stc_sub_length < stc_round_num_2_length) {
										if (i == stc_round[stc_length][0] && j == stc_round[stc_length][1]) {
											*(lcd_addr + k) = RED;
											stc_length++;
										}
										//Display Round Number On TFT-LCD
										elseif(delay_count > DELAY_ROUND_NUM&& i == stc_round_num_2[stc_sub_length][0] && j == stc_round_num_2[stc_sub_length][1]) {
											*(lcd_addr + k) = RED;
											stc_sub_length++;
										}
										else {
											*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
										}
									}
									else {
										*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
									}
								}
							}
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
							stc_length = 0;
							stc_sub_length = 0;
							delay_count++;
						}
						break;
					case 3:
						flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
						if (flag == 0) read_st = 0;
						if (flag == 1 && read_st == 0) {
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
							read_st = 1;
							for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
								for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
									k = i * CIS_IMAGE_WIDTH + j;
									//Display Round Characters and Round Number On TFT-LCD
									if (stc_length < stc_round_length || stc_sub_length < stc_round_num_3_length) {
										if (i == stc_round[stc_length][0] && j == stc_round[stc_length][1]) {
											*(lcd_addr + k) = RED;
											stc_length++;
										}
										//Display Round Number On TFT-LCD
										elseif(delay_count > DELAY_ROUND_NUM&& i == stc_round_num_3[stc_sub_length][0] && j == stc_round_num_3[stc_sub_length][1]) {
											*(lcd_addr + k) = RED;
											stc_sub_length++;
										}
										else {
											*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
										}
									}
									else {
										*(lcd_addr + k) = (unsignedshort)(*(cam_addr + k));
									}
								}
							}
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
							stc_length = 0;
							stc_sub_length = 0;
							delay_count++;
						}
						break;
					default:
						printf("DISPLAY_ROUND ERROR: CASE=DEFAULT\n");
						return;
						break;
					}
					//Delay Compare Check->Over? Countdown Start: Count again
					if (delay_count > DELAY_ROUND_STR) {
						state = COUNTDOWN_3;
						idle_flag = ACTIVE;
						delay_count = 0;
						break;
					}
				}
				break;

			case COUNTDOWN_3:
				//Countdown 3: Display Error Area And Countdown Number And Ring Buzzer
				if (idle_flag == ACTIVE) {
					idle_flag = IDLE;
					stc_length = 0;
					stc_sub_length = 0;
					printf("Countdown:	3\n");
					for (x = 0; x < DELAY_COUNTDOWN_PREDELAY;) {
						flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
						if (flag == 0) read_st = 0;
						if (flag == 1 && read_st == 0) {
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
							read_st = 1;
							for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
								for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
									k = i * CIS_IMAGE_WIDTH + j;
									Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));//Image Buffer<-Camera Image(Camera Image Data Debouncing)
									x++;
								}
							}
							__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;
						}
					}
					Ring_Buzzer_Short();
				}
				switch (round_num) {
				case 1:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));//Image Buffer<-Camera Image
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_1_display_length) {
							Image_Temp[stc_error_1_display[stc_length][1]][stc_error_1_display[stc_length][0]] = BLUE;//Image Buffer<-Error Area of Round 1
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_3_length) {
							Image_Temp[stc_countdown_num_3[stc_sub_length][1]][stc_countdown_num_3[stc_sub_length][0]] = RED;//Image Buffer<-Countdown Number 3
							stc_sub_length++;
						}
						//Initialize Index
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 2:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));//Image Buffer<-Camera Image
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_2_display_length) {
							Image_Temp[stc_error_2_display[stc_length][1]][stc_error_2_display[stc_length][0]] = BLUE;//Image Buffer<-Error Area of Round 2
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_3_length) {
							Image_Temp[stc_countdown_num_3[stc_sub_length][1]][stc_countdown_num_3[stc_sub_length][0]] = RED;//Image Buffer<-Countdown Number 3
							stc_sub_length++;
						}
						//Initialize Index
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 3:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));//Image Buffer<-Camera Image
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_3_display_length) {
							Image_Temp[stc_error_3_display[stc_length][1]][stc_error_3_display[stc_length][0]] = BLUE;//Image Buffer<-Error Area of Round 3
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_3_length) {
							Image_Temp[stc_countdown_num_3[stc_sub_length][1]][stc_countdown_num_3[stc_sub_length][0]] = RED;//Image Buffer<-Countdown Number 3
							stc_sub_length++;
						}
						//Initialize Index
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				default:
					printf("DISPLAY_ROUND ERROR: CASE=DEFAULT\n");
					return;
					break;
				}
				for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						k = i * CIS_IMAGE_WIDTH + j;
						*(lcd_addr + k) = Image_Temp[j][i];	//TFT-LCD<-Image Buffer
					}
				}
				//Delay Overflow Check->Over? Next State(Countdown_2):No Change
				if (delay_count > DELAY_COUNTDOWN) {
					state = COUNTDOWN_2;
					idle_flag = ACTIVE;
					delay_count = 0;
					break;
				}
				break;

			case COUNTDOWN_2://Almost Same Operation of Case: Countdown_3
				//Countdown 3: Display And Ring Buzzer
				if (idle_flag == ACTIVE) {
					idle_flag = IDLE;
					stc_length = 0;
					stc_sub_length = 0;
					printf("Countdown:	2\n");
					Ring_Buzzer_Short();
				}
				switch (round_num) {
				case 1:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_1_display_length) {
							Image_Temp[stc_error_1_display[stc_length][1]][stc_error_1_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_2_length) {
							Image_Temp[stc_countdown_num_2[stc_sub_length][1]][stc_countdown_num_2[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 2:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_2_display_length) {
							Image_Temp[stc_error_2_display[stc_length][1]][stc_error_2_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_2_length) {
							Image_Temp[stc_countdown_num_2[stc_sub_length][1]][stc_countdown_num_2[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 3:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_3_display_length) {
							Image_Temp[stc_error_3_display[stc_length][1]][stc_error_3_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_2_length) {
							Image_Temp[stc_countdown_num_2[stc_sub_length][1]][stc_countdown_num_2[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				default:
					printf("DISPLAY_ROUND ERROR: CASE=DEFAULT\n");
					return;
					break;
				}
				for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						k = i * CIS_IMAGE_WIDTH + j;
						*(lcd_addr + k) = Image_Temp[j][i];
					}
				}
				if (delay_count > DELAY_COUNTDOWN) {
					state = COUNTDOWN_1;
					idle_flag = ACTIVE;
					delay_count = 0;
					break;
				}
				break;

			case COUNTDOWN_1://Almost Same Operation of Case: Countdown_3
				if (idle_flag == ACTIVE) {
					idle_flag = IDLE;
					stc_length = 0;
					stc_sub_length = 0;
					printf("Countdown:	1\n");
					Ring_Buzzer_Short();
				}
				switch (round_num) {
				case 1:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_1_display_length) {
							Image_Temp[stc_error_1_display[stc_length][1]][stc_error_1_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_1_length) {
							Image_Temp[stc_countdown_num_1[stc_sub_length][1]][stc_countdown_num_1[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 2:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_2_display_length) {
							Image_Temp[stc_error_2_display[stc_length][1]][stc_error_2_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_1_length) {
							Image_Temp[stc_countdown_num_1[stc_sub_length][1]][stc_countdown_num_1[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				case 3:
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Temp[j][i] = (unsignedshort)(*(cam_addr + k));
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						while (stc_length < stc_error_3_display_length) {
							Image_Temp[stc_error_3_display[stc_length][1]][stc_error_3_display[stc_length][0]] = BLUE;
							stc_length++;
						}
						while (stc_sub_length < stc_countdown_num_1_length) {
							Image_Temp[stc_countdown_num_1[stc_sub_length][1]][stc_countdown_num_1[stc_sub_length][0]] = RED;
							stc_sub_length++;
						}
						stc_length = 0;
						stc_sub_length = 0;
						delay_count++;
					}
					break;
				default:
					printf("DISPLAY_ROUND ERROR: CASE=DEFAULT\n");
					return;
					break;
				}
				for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						k = i * CIS_IMAGE_WIDTH + j;
						*(lcd_addr + k) = Image_Temp[j][i];
					}
				}
				if (delay_count > DELAY_COUNTDOWN) {
					state = CAPTURE_IMAGE;
					idle_flag = ACTIVE;
					delay_count = 0;
					break;
				}
				break;

			case CAPTURE_IMAGE:	//Image Capture State
				Ring_Buzzer_Long();
				printf("Camera Image Capturing......");
				while (idle_flag == ACTIVE) {
					flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
					if (flag == 0) read_st = 0;
					if (flag == 1 && read_st == 0) {
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
						read_st = 1;
						for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
							for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
								k = i * CIS_IMAGE_WIDTH + j;
								Image_Capture[j][i] = (unsignedshort)(*(cam_addr + k));//Image Capture Buffer<-Camera Image
							}
						}
						__REG16(CIS_1300K_IRQ_ENABLE) = 0x01;//CAM->SRAM Enable
						idle_flag = IDLE;
					}
				}
				printf("Captured!\n");
				state = COMPARE;
				idle_flag = ACTIVE;
				delay_count = 0;
				break;

			case COMPARE:	//Compare Captured Image with Saved Image
				if (idle_flag == ACTIVE) {
					printf("Compare Start!\n");
					idle_flag = IDLE;
					Error_Count = 0;
					Error_Pixel_Count = 0;
					stc_length = 0;
					stc_sub_length = 0;
					delay_count = 0;
				}
				for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						//RGB to HSV(Calculate only for H value) Color-Space Conversion
						R_saved = (float)ABS(((Image_Saved[j][i] & 0xF800) >> 8) / 255.0);
						B_saved = (float)ABS(((Image_Saved[j][i] & 0x001F) << 3) / 255.0);
						G_saved = (float)ABS(((Image_Saved[j][i] & 0x07E0) >> 3) / 255.0);
						max_saved = MAX(MAX(R_saved, G_saved), B_saved);
						min_saved = MIN(MIN(R_saved, G_saved), B_saved);
						dt_saved = max_saved - min_saved;
						if (max_saved == R_saved) {
							H_saved = 60 * ((G_saved - B_saved) / dt_saved);
						}
						elseif(max_saved == G_saved) {
							H_saved = 60 * ((B_saved - R_saved) / dt_saved + 2.0);
						}
						elseif(max_saved == B_saved) {
							H_saved = 60 * ((R_saved - G_saved) / dt_saved + 4.0);
						}
						else {
							printf("RGB2HSV Converting ERROR: SAVED DATA UNDEFINED VALUE\n");
						}
						R_captured = (float)ABS(((Image_Capture[j][i] & 0xF800) >> 8) / 255.0);
						G_captured = (float)ABS(((Image_Capture[j][i] & 0x07E0) >> 3) / 255.0);
						B_captured = (float)ABS(((Image_Capture[j][i] & 0x001F) << 3) / 255.0);
						max_captured = MAX(MAX(R_captured, G_captured), B_captured);
						min_captured = MIN(MIN(R_captured, G_captured), B_captured);
						dt_captured = max_captured - min_captured;
						if (max_captured == R_captured) {
							H_captured = 60 * ((G_captured - B_captured) / dt_captured);
						}
						elseif(max_captured == G_captured) {
							H_captured = 60 * ((B_captured - R_captured) / dt_captured + 2.0);
						}
						elseif(max_captured == B_captured) {
							H_captured = 60 * ((R_captured - G_captured) / dt_captured + 4.0);
						}
						else {
							printf("RGB2HSV Converting ERROR:  CAPTURED DATA UNDEFINED VALUE\n");
						}
						//image Error Data Input
						Image_Error[j][i] = ABS(H_saved - H_captured);
					}
				}
				//ERROR CHECK
				switch (round_num) {
					//Image Error Check for Each Round's Error Area
				case 1:
					//ROUND 1
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < (CIS_IMAGE_WIDTH); j++) {
							if (j < (CIS_IMAGE_WIDTH / 2)) {
								if (Image_Error[j][i] > ERROR_THRESHOLD) {
									Error_Count++;//Check Pixels over ERROR_THRESHOLD on Error Area
								}
								else {
									Image_Error[j][i] = 0;
								}
								Error_Pixel_Count++;//Check Every Pixel on Error Area
							}
							else {
								Image_Error[j][i] = 0;
							}
						}
					}
					printf("Error is %d / %d\n", Error_Count, Error_Pixel_Count);
					break;
				case 2:
					//ROUND 2
					//Do same for Round 1
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							if (j < 120) {
								if (Image_Error[j][i] > ERROR_THRESHOLD) {
									Error_Count++;
								}
								else {
									Image_Error[j][i] = 0;
								}
								Error_Pixel_Count++;
							}
							elseif(j > 200) {
								if (Image_Error[j][i] > ERROR_THRESHOLD) {
									Error_Count++;
								}
								else {
									Image_Error[j][i] = 0;
								}
								Error_Pixel_Count++;
							}
							else Image_Error[j][i] = 0;
						}
					}
					printf("Error is %d / %d\n", Error_Count, Error_Pixel_Count);
					break;
				case 3:
					//ROUND 3
					//Do same for Round 3
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						for (i = 0; i < (CIS_IMAGE_HEIGHT); i++) {
							if ((i < 60) || ((i >= 60 && i < 180) && (j < 80 || j>240)) || ((i >= 180) && (j <= 100 || j >= 220))) {
								if (Image_Error[j][i] > ERROR_THRESHOLD) {
									Error_Count++;
								}
								else {
									Image_Error[j][i] = 0;
								}
								Error_Pixel_Count++;
								stc_length++;
							}
							else {
								Image_Error[j][i] = 0;
							}
						}
					}
					printf("Error is %d / %d\n", Error_Count, Error_Pixel_Count);
					break;
				default:
					printf("STATE: COMPARE, CASE:DEFAULT ERROR!\n");
					return;
					break;
				}
				//Show Error Image Pixel
				printf("Print Error Image!\n");
				for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
					for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
						k = i * CIS_IMAGE_WIDTH + j;

						if (Image_Error[j][i] != 0) {
							*(lcd_addr + k) = RED;	//if Pixel is Errored Pixel, Display RED
						}
						else {
							(*(lcd_addr + k)) = Image_Capture[j][i];	//if Pixel is Not-Errored, Display Captured Image
						}
					}
				}
				Error_Percentage = Error_Count * 100.0 / Error_Pixel_Count;	//Calculate Error Percentage(Errored Pixels Count/Every Error-Checked Area Pxels Count)
				Point_Sum += (int)(difficulty_point_weight * (100 - ((int)Error_Percentage)) * POINT_RAND);	//Get Gameplay Point

				printf("Error Percentage: %d Percent\n", (int)(Error_Percentage));
				msWait(DELAY_ERROR_IMAGE_DISPLAY);
				stc_length = 0;
				stc_sub_length = 0;
				//Error Percentage Check->Fail or Success or Congratulations(Victory Every Round)
				if (Error_Percentage >= PERCENT_THRESHOLD) {
					state = DISPLAY_FAIL;
					idle_flag = ACTIVE;
				}
				else {
					round_num++;
					if (round_num > MAXIMUM_ROUND) {
						state = DISPLAY_ALL_WIN;
						idle_flag = ACTIVE;
					}
					else {
						state = DISPLAY_WIN;
						idle_flag = ACTIVE;
					}
				}
				textlcd_score(round_num, (int)Point_Sum);//Point Data+Round Data->Display on Character LCD/LED
				break;

			case DISPLAY_FAIL:
				//Fail Graphic Display
				if (idle_flag == ACTIVE) {
					printf("Fail...\n\n");
					idle_flag = IDLE;
					Ring_Buzzer_Short();
					Ring_Buzzer_Short();
				}
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							if (stc_length < stc_fail_length) {
								if (i == stc_fail[stc_length][0] && j == stc_fail[stc_length][1]) {
									*(lcd_addr + k) = (unsignedshort)RED;//Display "FAIL..." Characters
									stc_length++;
								}
								else {
									*(lcd_addr + k) = (unsignedshort)0x0000;//Black Color Display
								}
							}
							else {
								*(lcd_addr + k) = (unsignedshort)0x0000;//Black Color Display
							}
						}
					}
					if (delay_count > DELAY_GAME_END) {
						state = MAIN_MENU;	//Go back to Main Menu
						idle_flag = ACTIVE;
						delay_count = 0;
					}
					delay_count++;
					stc_length = 0;
					stc_sub_length = 0;
				}
				break;

			case DISPLAY_WIN:
				//Win Graphic Display -> Next Round
				if (idle_flag == ACTIVE) {
					printf("Success!\n\n");
					idle_flag = IDLE;
					Ring_Buzzer_Long();
				}
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							if (stc_length < stc_success_length) {
								if (i == stc_success[stc_length][0] && j == stc_success[stc_length][1]) {
									*(lcd_addr + k) = (unsignedshort)RED;	//Display "SUCCESS!!" Characters
									stc_length++;
								}
								else {
									*(lcd_addr + k) = (unsignedshort)0xFFFF;	//DISPLAY WHITE
								}
							}
							else {
								*(lcd_addr + k) = (unsignedshort)0xFFFF;	//DISPLAY WHITE
							}
						}
					}
					if (delay_count > DELAY_GAME_END) {
						state = ROUND_NUM_DISPLAY;
						idle_flag = ACTIVE;
						delay_count = 0;
					}
					delay_count++;
					stc_length = 0;
					stc_sub_length = 0;
				}
				break;

			case DISPLAY_ALL_WIN:
				//DISPLAY Congratulations Graphic on TFT-LCD
				if (idle_flag == ACTIVE) {
					printf("Congratulations!\nYou Win!\n\n");
					idle_flag = IDLE;
					Ring_Buzzer_Long();
					Ring_Buzzer_Long();
					Ring_Buzzer_Long();
				}
				flag = (__REG(GPIO_BASE + GPLR1) & 0x04000000) >> 26;  //GPIO[58] // P320
				if (flag == 0) read_st = 0;
				if (flag == 1 && read_st == 0) {
					__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;//CAM->SRAM DISABLE
					read_st = 1;
					for (i = 0; i < CIS_IMAGE_HEIGHT; i++) {
						for (j = 0; j < CIS_IMAGE_WIDTH; j++) {
							k = i * CIS_IMAGE_WIDTH + j;
							if (stc_length < stc_congratulation_length) {
								if (i == stc_congratulation[stc_length][0] && j == stc_congratulation[stc_length][1]) {
									*(lcd_addr + k) = (unsignedshort)VIOLET;	//Display "Congratulations!!" Characters
									stc_length++;
								}
								else {
									*(lcd_addr + k) = (unsignedshort)0xFFFF;	//Display WHITE
								}
							}
							else {
								*(lcd_addr + k) = (unsignedshort)0xFFFF;	//Display White
							}
						}
					}
					if (delay_count > DELAY_GAME_END) {
						state = MAIN_MENU;//Go back To Main Menu
						idle_flag = ACTIVE;
						delay_count = 0;
					}
					delay_count++;
					stc_length = 0;
					stc_sub_length = 0;
				}
				break;

			default:
				printf("Entered State=Default\n");
				break;
			}//Switch(state) End
		}//Else End
		if (GetChar(&ch) == 1) {	//If Keyboard Input==Q or q -> End Program
			if (ch == 'Q')break;
			if (ch == 'q')break;
		}
		old = key_value;//Keypad Debouncing
	}//While End
	__REG16(CIS_1300K_IRQ_ENABLE) = 0x00;
}

unsignedchar key_scan(unsignedchar* key_addr, unsignedchar* key_addr_rd)//Keypad Input Scanning Function
{
	unsignedchar key_value;
	int i;
	*key_addr = 0x01;
	for (i = 0; i < 10; i++);
	key_value = *key_addr_rd;
	key_value &= 0x0f;
	if (key_value > 0) {
		if (key_value == 0x01)key_value = 1;
		if (key_value == 0x02)key_value = 2;
		if (key_value == 0x04)key_value = 3;
		if (key_value == 0x08)key_value = 4;
		gotoscan_end;
	}
	*key_addr = 0x02;
	for (i = 0; i < 10; i++);
	key_value = *key_addr_rd;
	key_value &= 0x0f;
	if (key_value > 0) {
		if (key_value == 0x01)key_value = 5;
		if (key_value == 0x02)key_value = 6;
		if (key_value == 0x04)key_value = 7;
		if (key_value == 0x08)key_value = 8;
		gotoscan_end;
	}
	*key_addr = 0x04;
	for (i = 0; i < 10; i++);
	key_value = *key_addr_rd;
	key_value &= 0x0f;
	if (key_value > 0) {
		if (key_value == 0x01)key_value = 9;
		if (key_value == 0x02)key_value = 10;
		if (key_value == 0x04)key_value = 11;
		if (key_value == 0x08)key_value = 12;
		gotoscan_end;
	}
	*key_addr = 0x08;
	for (i = 0; i < 10; i++);
	key_value = *key_addr_rd;
	key_value &= 0x0f;
	if (key_value > 0) {
		if (key_value == 0x01)key_value = 13;
		if (key_value == 0x02)key_value = 14;
		if (key_value == 0x04)key_value = 15;
		if (key_value == 0x08)key_value = 16;
		gotoscan_end;
	}
scan_end:
	return key_value;
}
void Display_Main_Menu(void) {//Main Menu Graphic Display Function
	int x, y;
	for (x = 0; x < CIS_IMAGE_WIDTH; x++) {
		for (y = 0; y < CIS_IMAGE_HEIGHT; y++) {
			*(lcd_addr + y * CIS_IMAGE_WIDTH + x) = image_main[x * CIS_IMAGE_HEIGHT + y];	//Main Menu Graphic Data->TFT-LCD
		}
	}
}
void Display_How_To_Play(void) {//How To Play Graphic Display Function
	int x, y;
	for (x = 0; x < CIS_IMAGE_WIDTH; x++) {
		for (y = 0; y < CIS_IMAGE_HEIGHT; y++) {
			*(lcd_addr + y * CIS_IMAGE_WIDTH + x) = image_howto[x * CIS_IMAGE_HEIGHT + y];	//How To Play Graphic Data->TFT-LCD
		}
	}
}
void Display_Select_Difficulty(void) {//Select Difficulty Graphic Display Function
	int x, y;
	for (x = 0; x < CIS_IMAGE_WIDTH; x++) {
		for (y = 0; y < CIS_IMAGE_HEIGHT; y++) {
			*(lcd_addr + y * CIS_IMAGE_WIDTH + x) = image_select_difficulty[x * CIS_IMAGE_HEIGHT + y];	//Select Difficulty Graphic Data->TFT-LCD
		}
	}
}
void Ring_Buzzer_Short(void) {	//Ring Buzzer Function: Short
	*buzzer_addr = 0x01;
	msWait(DELAY_BUZZER_SHORT);
	*buzzer_addr = 0x00;
	msWait(DELAY_BUZZER_SHORT);
}
void Ring_Buzzer_Long(void) {	//Ring Buzzer Function: Long
	*buzzer_addr = 0x01;
	msWait(DELAY_BUZZER_LONG);
	*buzzer_addr = 0x00;
	msWait(DELAY_BUZZER_SHORT);
}
//Character LCD Funcitons
void setcommand(unsignedshort command)
{

	*Clcd_addr = command | 0x0100;
	Wait(300);    	// 300us
	*Clcd_addr = command | 0x0000;
	Wait(300);    	// 300us
}
void writebyte(char ch)
{
	unsignedshort data;
	data = ch & 0x00FF;

	*Clcd_addr = data | 0x500;   // dvk270peri  .eq. bulverde
	Wait(300);    	// 300us

	*Clcd_addr = data | 0x400;
	Wait(300);    	// 300us
}
int function_set(int rows, int nfonts) {
	unsignedshort command = 0x30;

	if (rows == 2) command |= 0x08;
	elseif(rows == 1) command &= 0xf7;
	elsereturn - 1;

	command = nfonts ? (command | 0x04) : command;
	setcommand(command);
	return1;
}
int display_control(int display_enable, int cursor_enable, int nblink) {
	unsignedshort command = 0x08;
	command = display_enable ? (command | 0x04) : command;
	command = cursor_enable ? (command | 0x02) : command;
	command = nblink ? (command | 0x01) : command;
	setcommand(command);
	return1;
}
int cusrsor_shit(int set_screen, int set_rightshit) {
	unsignedshort command = 0x10;
	command = set_screen ? (command | 0x08) : command;
	command = set_rightshit ? (command | 0x04) : command;
	setcommand(command);
	return1;
}
int entry_mode_set(int increase, int nshift) {
	unsignedshort command = 0x04;
	command = increase ? (command | 0x2) : command;
	command = nshift ? (command | 0x1) : command;
	setcommand(command);
	return1;
}
int return_home() {
	unsignedshort command = 0x02;
	setcommand(command);
	Wait(300);    	// 300us
	return1;
}
int clear_display() {
	unsignedshort command = 0x01;
	setcommand(command);
	Wait(300);    	// 300us
	return1;
}
int set_ddram_address(int pos) {
	unsignedshort command = 0x80;
	command += pos;
	setcommand(command);
	return1;
}
void initialize_textlcd() {
	function_set(2, 0);//Function Set:8bit,display 2lines,5x7 mod
	display_control(1, 0, 0);// Display on, Cursor off
	clear_display();// Display clear
	return_home();// go home
	entry_mode_set(1, 0);// Entry Mode Set : shift right cursor
	Wait(300);    	// 300us
}
void textlcd_score(int round, int score) {	//Round Data+Score Data->Display On LED, Character LCD
	Clcd_addr = (unsignedshort*)ADDR_OF_CLCD;
	if (round > 3) {
		round = 3;
	}
	led_addr = (unsignedchar*)ADDR_OF_LED;
	*led_addr = (round & 0xFF);
	char rnd = round + '0';
	int temp1, temp2, temp3, temp4;
	char scr[6];
	scr[0] = (score / 100000) + '0';
	temp1 = score % 100000;
	scr[1] = (temp1 / 10000) + '0';
	temp2 = score % 10000;
	scr[2] = (temp2 / 1000) + '0';
	temp3 = score % 1000;
	scr[3] = (temp3 / 100) + '0';
	temp4 = score % 100;
	scr[4] = (temp4 / 10) + '0';
	scr[5] = (score % 10) + '0';
	int i;
	char line1[16];
	char line2[16];
	line1[0] = 'R'; line1[1] = 'o'; line1[2] = 'u'; line1[3] = 'n';
	line1[4] = 'd'; line1[5] = ' '; line1[6] = ' '; line1[7] = rnd;
	line1[8] = ' '; line1[9] = ' '; line1[10] = ' '; line1[11] = ' ';
	line1[12] = ' '; line1[13] = ' '; line1[14] = ' '; line1[15] = ' ';
	line2[0] = 'S';
	line2[1] = 'c';
	line2[2] = 'o';
	line2[3] = 'r';
	line2[4] = 'e';
	line2[5] = ' ';
	line2[6] = '0';
	line2[7] = scr[1];
	line2[8] = scr[2];
	line2[9] = scr[3];
	line2[10] = scr[4];
	line2[11] = scr[5];
	line2[12] = ' '; line2[13] = ' '; line2[14] = ' '; line2[15] = ' ';
	initialize_textlcd();
	Wait(500);    	// 300us
	for (i = 0; i < 16; i++) writebyte(line1[i]);
	set_ddram_address(0x40);
	for (i = 0; i < 16; i++) writebyte(line2[i]);
}
