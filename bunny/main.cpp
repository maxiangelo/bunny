#include <Windows.h>
#include <iostream>
#include <vector>
#include <math.h>
#include "connector.h"
CHackProcess fProcess;

constexpr auto onGround = 257;
constexpr auto crouchedGround = 263;

const DWORD dwPlayerBase = 0xCF2A3C;
const DWORD dwJump = 0x51A81DC;
const DWORD entityList = 0x4D04B24;
const DWORD m_fFlags = 0x104;
const DWORD flashAlpha = 0xA3F0;
const DWORD isSpotted = 0x93D;
const DWORD dwCham = 0x70;
const DWORD dwTeam = 0xF4;
const DWORD dwCrossID = 0xB3AC;
const DWORD dwModelAmb = 0x58FD1C; //for brightness
const DWORD dwLeft = 0x313624C; //for auto strafe
const DWORD dwRight = 0x3136240; //for auto strafe
const DWORD bSendPackets = 0xD280A; //for lag (in BYTE!)
const DWORD dwHealth = 0x100; 
const DWORD dwMouseEnable = 0xCF8588; //for not jumping when chatting in window
const DWORD dwVecVelocity = 0x114;


bool bBhop = false;
bool bflash = false;
bool bRadar = false;
bool bChams = false;
bool bTrigger = false;
bool bFakeL = false;

struct myPlayer_T
{
	DWORD dwLocalP = 0x0;
	int flag = 0;
	int flash = 0;
	int iTeam = 0;
	int iHealth = 0;
	int iCrossID = 0;

	std::vector<float> vel;

	
	void ReadInfo()
	{
		//Read this all the time..
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dwPlayerBase), &dwLocalP, sizeof(DWORD), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + m_fFlags), &flag, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + flashAlpha), &flash, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + dwTeam), &iTeam, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + dwCrossID), &iCrossID, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + dwHealth), &iHealth, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(dwLocalP + dwVecVelocity), &vel, sizeof(vel), 0);
	}
} myPlayer;

void bunny()
{
	if (bBhop)
	{
		int doJump = 5;
		int stopJump = 4;
		byte bMouseEnabled = 0; //88 = mouse off, 89 = mouse on

		ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dwMouseEnable), &bMouseEnabled, sizeof(bMouseEnabled), 0);
		//only when alive and moving
		if ((myPlayer.flag == onGround && myPlayer.iHealth > 0 && bMouseEnabled == 89 && myPlayer.vel != std::vector<float>(0.0f)) ||
			(myPlayer.flag == crouchedGround && myPlayer.iHealth > 0 && bMouseEnabled == 89 && myPlayer.vel != std::vector<float>(0.0f))
			)
		{
			WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dwJump), &doJump, sizeof(doJump), nullptr);
		}
		else
		{
			WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + dwJump), &stopJump, sizeof(stopJump), nullptr);
		}
	}
}

void flash()
{
	float newAlphaFlash = .2f;

	if (bflash)
	{
		if (myPlayer.flash > .5f)
		{
			WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(myPlayer.dwLocalP + flashAlpha), &newAlphaFlash, sizeof(newAlphaFlash), nullptr);
		}
	}
}

void radar()
{
	if (bRadar)
	{
		DWORD entity = 0x0;
		bool bSpot = true;

		for (short i = 0; i < 64; i++)
		{
			//Radar
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + entityList + (i * 0x10)), &entity, sizeof(DWORD), nullptr);

			//Spot
			if (entity != NULL)
			{
				WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + isSpotted), &bSpot, sizeof(bool), nullptr);
			}
		}
	}
}

void drawChams()
{
	if (bChams)
	{
		DWORD entity = 0x0;

		//actually EntityTeam
		int enemyTeam = 0;

		//no need for alpha value because brightness does the work for us
		byte rgbColor[3] = {33, 103, 255}; //cyan
		byte rgbColorEnemy[3] = {255, 25, 155}; //pink
		float brightness = 15.f; //65 is very bright
		DWORD thisPtr = (int)(fProcess.__dwordEngine + dwModelAmb - 0x2c);
		DWORD xored = *(DWORD*)&brightness ^ thisPtr;

		for (int i = 0; i < 64; i++)
		{
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + entityList + (i * 0x10)),&entity, sizeof(DWORD), nullptr);
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwTeam), &enemyTeam, sizeof(int), nullptr);

			if (entity != NULL && enemyTeam != myPlayer.iTeam)
			{
				WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemy, sizeof(rgbColorEnemy), nullptr);
			}
			else if (entity != NULL && enemyTeam == myPlayer.iTeam)
			{
				WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColor, sizeof(rgbColor), nullptr);
			}
		}

		WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + dwModelAmb), &xored, sizeof(int),nullptr);
	}
	else
	{
		//reset
		DWORD entity = 0x0;
		int enemyTeam = 0;
		byte rgbColor[3] = {255, 255, 255};
		byte rgbColorEnemy[3] = {255, 255, 255};
		float resetBrightness = 0.f;
		const DWORD thisPtr = (int)(fProcess.__dwordEngine + dwModelAmb - 0x2c);
		DWORD resetXored = *(DWORD*)&resetBrightness ^ thisPtr;

		for (int i = 0; i < 64; i++)
		{
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordClient + entityList + (i * 0x10)),&entity, sizeof(DWORD), nullptr);

			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwTeam), &enemyTeam, sizeof(int), nullptr);

			if (entity != NULL && enemyTeam != myPlayer.iTeam)
			{
				WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColorEnemy, sizeof(rgbColorEnemy), nullptr);
			}
			else if (entity != NULL && enemyTeam == myPlayer.iTeam)
			{
				WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(entity + dwCham), &rgbColor, sizeof(rgbColor), nullptr);
			}
		}

		WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + dwModelAmb), &resetXored, sizeof(int), nullptr);
	}
}

void trigger()
{
	DWORD aimedEntity = 0x0;
	DWORD entity = 0x0;
	int enemyTeam = 0;

	if (bTrigger)
	{
		const int AIMED_AT = myPlayer.iCrossID;

		if (AIMED_AT > 0 && AIMED_AT < 64)
		{
			ReadProcessMemory(fProcess.__HandleProcess,
			                  (PBYTE*)(fProcess.__dwordClient + entityList + (AIMED_AT - 1) * (0x10)), &aimedEntity,
			                  sizeof(DWORD), nullptr);
		}

		for (int i = 0; i < 64; i++)
		{
			ReadProcessMemory(fProcess.__HandleProcess, (PBYTE*)(aimedEntity + dwTeam), &enemyTeam, sizeof(int), nullptr);
		}

		if (myPlayer.iCrossID > 0 && enemyTeam != myPlayer.iTeam)
		{
			Sleep(14);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			Sleep(10);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		}
	}
}

void fakeLag()
{
	byte lagON = 0;
	byte lagFalse = 1;

	if (bFakeL)
	{
		WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + bSendPackets), &lagON,sizeof(byte), nullptr);

		Sleep(125); //1000ms/64 tick = 15.6 ..  15.6 * 8 Bit = 125
		WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + bSendPackets), &lagFalse,sizeof(byte), nullptr);
	}
	else
	{
		WriteProcessMemory(fProcess.__HandleProcess, (PBYTE*)(fProcess.__dwordEngine + bSendPackets), &lagFalse, sizeof(byte), nullptr);
	}
}

void onSound()
{
	Beep(250, 200);
}

void offSound()
{
	Beep(400, 200);
}

void playSound(const bool state)
{
	if (!state)
	{
		onSound();
	}
	else
	{
		offSound();
	}
}

int main(void)
{
	//always forgetting this line...
	fProcess.RunProcess();

	//if window not found
	if (fProcess.__HWNDCSgo == nullptr)
	{
		std::cout << "Process 'csgo.exe' not found!" << std::endl;
		std::cout << "Last Error: " << GetLastError() << std::endl;
	}
	else
	{
		std::cout << "Bunny by c1tru5x" << std::endl;
		std::cout << "Updated 19.JUN.2019" << std::endl;
		std::cout << "F11 to close!" << std::endl;
		std::cout << "[NUM1] BHOP use SPACE" << std::endl;
		std::cout << "[NUM2] No Flash!" << std::endl;
		std::cout << "[NUM3] Radar" << std::endl;
		std::cout << "[NUM4] Chams" << std::endl;
		std::cout << "[NUM5] Trigger use ALT" << std::endl;

		while (!GetAsyncKeyState(VK_F11))
		{
			myPlayer.ReadInfo();

			if (GetAsyncKeyState(VK_NUMPAD1))
			{
				bBhop = !bBhop;

				playSound(bBhop);			
			}

			if (GetAsyncKeyState(VK_NUMPAD2))
			{
				bflash = !bflash;

				playSound(bflash);
			}

			if (GetAsyncKeyState(VK_NUMPAD3))
			{
				bRadar = !bRadar;

				playSound(bRadar);
			}

			if (GetAsyncKeyState(VK_NUMPAD4))
			{
				bChams = !bChams;

				playSound(bChams);
			}

			if (GetAsyncKeyState(VK_NUMPAD5))
			{
				bTrigger = !bTrigger;

				playSound(bTrigger);
			}

			if (GetAsyncKeyState(VK_NUMPAD6))
			{
				bFakeL = !bFakeL;

				playSound(bFakeL);
			}

			// Function call
			if (bTrigger)
			{
				if (GetAsyncKeyState(VK_LMENU))
				{
					trigger();
				}
			}

			if (bBhop)
			{
				if (GetAsyncKeyState(VK_SPACE))
				{
					bunny();
				}
			}

			flash();
			radar();
			drawChams();
			fakeLag();

			Sleep(1);
		}
	}
	return 0;
}
