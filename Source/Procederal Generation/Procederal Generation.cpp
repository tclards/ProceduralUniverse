#define OLC_PGE_APPLICATION
#include "..//olcPixelGameEngine-master//olcPixelGameEngine.h"
#include "..//olcPixelGameEngine-master//olcPGEX_Sound.h"

// TODO -
// weight random gens to create logical systems.
// more complex draw logic for planet color -- or add sprites?
// name ganerator for planets and systems
// TODO - planet stuff -
			// weight diameter on gaussian curve based on distance from star
			// weight temperature based on "size == hotter && distance from star == colder"
			// implement bool bHasSurfaceDetails based on closer to sun and smaller size
			// tie foliage and water into bHasSurfaceDetails
			// random chance for moon to have surface detail too - will also need draw logic
// add looping music?
// planetary inspection screen
// fill out readme

// Controls:
// WASD or Arrow Keys	-						for panning camera around Galaxy
// Mouse Left Click		-						to select and view Star System. Click anywhere besides a star to close window.

#pragma region Structs and Global Variables
// define possible star colors
constexpr uint32_t g_starColors[8] =
{
	0xFFFFFFFF, 0xFFD9FFFF, 0xFFA3FFFF, 0xFFFFC8C8,
	0xFFFFCB9D, 0xFF9F9FFF, 0xFF415EFF, 0xFF28199D
};

// struct for defining planetary data
struct sPlanet
{
	double distance = 0.0f;
	double diameter = 0.0f;
	double foliage = 0.0f;
	double water = 0.0f;
	double temperature = 0.0f;
	bool ring = false;
	std::vector<double> vMoons;
};
#pragma endregion

// class to define what exists at each location in universe
#pragma region Star System Logic
class cStarSystem 
{
public:
	bool		bStarExists = false;
	double		dStarDiameter = 0.0f;
	olc::Pixel  starColor = olc::WHITE;
	std::vector<sPlanet> vPlanets;
	bool		bHasAsteroidBelt = false;

public:
	cStarSystem(uint32_t x, uint32_t y, bool bGenerateFullSystem = false) // constructor
	{
		// seed random generation and set boundaries of universe (non-infinite, just very big)
		nLehmer = (x & 0xFFFF) << 16 | (y & 0xFFFF);

		// draw a random int between 0 and 20. If equal to 1, set to true. 1 in 20 locations contain a star
		bStarExists = (rndInt(0, 20) == 1);

		// if star does not exist, end function for this location and move on to next
		if (!bStarExists) return;

		// generate star color and size
		dStarDiameter = rndDouble(10.0f, 40.0f);
		starColor.n = g_starColors[rndInt(0, 8)];

		// set asteroid belt flag
		bHasAsteroidBelt = (rndInt(0, 6) == 1); // 1 in 6 systems has an asteroid belt


		// if we have selected the star and are examining it, we need to generate planets and details, otherwise, end the function here
		if (!bGenerateFullSystem) return;

		double dDistanceFromStar = rndDouble(60.0f, 200.0f); // generate distance between star and first planet
		int nPlanets = rndInt(0, 10); // determine number of planets in system
		for (int i = 0; i < nPlanets; i++) // iterate over planets
		{
			sPlanet p;
			p.distance = dDistanceFromStar;					// set distance from star
			dDistanceFromStar += rndDouble(20.0f, 200.0f);  // add to distance for next loop
			p.diameter = rndDouble(4.0f, 20.0f);			// set planet size
			p.temperature = rndDouble(-200.0f, 300.0f);		// Set planet temperature

			p.foliage = rndDouble(0.0f, 1.0f);				// calculate percentage of water vs foliage
			p.water = rndDouble(0.0f, 1.0f);
			double dSum = 1.0f / (p.foliage + p.water);
			p.foliage *= dSum;
			p.water *= dSum;

			p.ring = (rndInt(0, 10) == 1);					// 1 in 10 planets has a ring
		
			int nMoons = std::max(rndInt(-5, 5), 0);		// determine planets moons
			for (int n = 0; n < nMoons; n++)
			{
				p.vMoons.push_back(rndDouble(1.0f, 5.0f));
			}

			vPlanets.push_back(p);							// Add to systems vector of planets now that generation is complete
		}
	}

private: // Utility Functions
	uint32_t nLehmer = 0;
	uint32_t Lehmer32() // Lehmer Random Number Algorithm
	{
		nLehmer += 0xe120fc15;
		uint64_t tmp;
		tmp = (uint64_t)nLehmer * 0x4a39b70d;
		uint32_t m1 = (tmp >> 32) ^ tmp;
		tmp = (uint64_t)m1 * 0x12fad5c9;
		uint64_t m2 = (tmp >> 32) ^ tmp;
		return m2;
	}

	int rndInt(int min, int max) // Generate random integer between a specified range
	{
		return (Lehmer32() % (max - min)) + min;
	}
	double rndDouble(double min, double max) // Generate random double between a specified range
	{
		return ((double)Lehmer32() / (double)(0x7FFFFFFF) * (max - min) + min);
	}
};
#pragma endregion

// game/sim logic
#pragma region Galaxy Logic
class Galaxy : public olc::PixelGameEngine
{
public:
	Galaxy()
	{
		sAppName = "Galaxy";
	}

	// 2D vector representing 'camera' location inside universe
	olc::vf2d vGalaxyOffset = { 0,0 };

	// Flag for if a star is selected
	bool bStarSelected = false;

	// vector for caching selected system's location
	olc::vi2d vStarSelected = { 0,0 };

public:
	bool OnUserCreate() override
	{
		// Start music loop


		return true;
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		// User Input - camera
		if (GetKey(olc::Key::W).bHeld || GetKey(olc::Key::UP).bHeld) vGalaxyOffset.y -= 50.0f * fElapsedTime;
		if (GetKey(olc::Key::S).bHeld || GetKey(olc::Key::DOWN).bHeld) vGalaxyOffset.y += 50.0f * fElapsedTime;
		if (GetKey(olc::Key::A).bHeld || GetKey(olc::Key::LEFT).bHeld) vGalaxyOffset.x -= 50.0f * fElapsedTime;
		if (GetKey(olc::Key::D).bHeld || GetKey(olc::Key::RIGHT).bHeld) vGalaxyOffset.x += 50.0f * fElapsedTime;

		// create sectors
		int sectorSize = 16; // number of sectors down & across screen
		int nSectorX = ScreenWidth() / sectorSize;
		int nSectorY = ScreenHeight() / sectorSize;

		// User Input - Mouse
		olc::vi2d mouse = { GetMouseX() / sectorSize, GetMouseY() / sectorSize}; // determine mouse onscreen location - which sector is it in?
		olc::vi2d galaxy_mouse = mouse + vGalaxyOffset;	// determine mouse galactic location - where is it relative to whole galaxy
		
		// vector for holding coordinates of a particular sector on the screen
		olc::vi2d screen_sector = { 0,0 };

		// Clear screen to black at start of each frame
		Clear(olc::BLACK);

		// iterate over sectors visible on screen
		for (screen_sector.x = 0; screen_sector.x < nSectorX; screen_sector.x++)
		{
			for (screen_sector.y = 0; screen_sector.y < nSectorY; screen_sector.y++)
			{
				// construct star system object for this location, 
				// accounts for offset('camera' location on screen - controlled by user input)
				cStarSystem star(screen_sector.x + (uint32_t)vGalaxyOffset.x,
					screen_sector.y + (uint32_t)vGalaxyOffset.y);

				// first check if star exists here
				if (star.bStarExists)
				{
					// if star exists, draw circle using stars data and location
					FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8,
						(int)star.dStarDiameter / 8, star.starColor);

					// if mouse cursor is hovering over star - highlight it
					if (mouse.x == screen_sector.x && mouse.y == screen_sector.y)
					{
						DrawCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8, 12, olc::YELLOW);
					}
				}
			}
		}

		// User Input - Mouse Click
		if (GetMouse(0).bPressed)
		{
			// generate system that was clicked on
			cStarSystem star(galaxy_mouse.x, galaxy_mouse.y);

			// check if system exists here
			if (star.bStarExists)
			{
				bStarSelected = true;			// set flag for tracking when a system is selected
				vStarSelected = galaxy_mouse;	// cache location
			}
			else
			{
				bStarSelected = false;			// set flag for tracking when a system is selected
			}
		}

		// if a system has been selected - we need to draw its details
		if (bStarSelected)
		{
			// generate full system and planetary details
			cStarSystem star(vStarSelected.x, vStarSelected.y, true);

			int nMiddle = 0;
			if (star.bHasAsteroidBelt)
			{
				int nPlanetCount = star.vPlanets.size();
				nMiddle = (nPlanetCount / 2) + 0.5f;
			}

			// Draw Window
			FillRect(8, 240, 496, 232, olc::DARK_BLUE);
			DrawRect(8, 240, 496, 232, olc::WHITE);

			// Draw Star
			olc::vi2d vBody = { 14, 356 };
			vBody.x += star.dStarDiameter * 1.375f;
			FillCircle(vBody, (int)(star.dStarDiameter * 1.375), star.starColor);
			vBody.x += (star.dStarDiameter * 1.375) + 8;

			// Draw Planets and Moons
			for (int i = 0; i < star.vPlanets.size(); i++)
			{
				// check for clipping
				if (vBody.x + star.vPlanets[i].diameter >= 496 - 48)
				{
					break;
				}

				// draw asteroid belt
				if (star.bHasAsteroidBelt && nMiddle == i)
				{
					olc::vi2d vTmp = vBody;
					vTmp.x += 4;
					FillRect(vTmp.x, 241, 8, 231, olc::DARK_YELLOW);


					vBody.x += 20;
				}

				// create planet color based on temperature, as well as water vs foliage details
				bool bSelectingColor = true;
				olc::Pixel tmp = olc::GREY; // default to grey in case of errors
				if (star.vPlanets[i].temperature < 50)
				{
					tmp = olc::WHITE;
					bSelectingColor = false;
				}
				else if (star.vPlanets[i].temperature > 125)
				{
					tmp = olc::RED;
					bSelectingColor = false;
				}

				if (bSelectingColor == true && star.vPlanets[i].foliage > star.vPlanets[i].water)
				{
					tmp = olc::DARK_GREEN;
					bSelectingColor = false;
				}
				else if (bSelectingColor == true && star.vPlanets[i].foliage < star.vPlanets[i].water)
				{
					tmp = olc::BLUE;
					bSelectingColor = false;
				}

				vBody.x += star.vPlanets[i].diameter;
				FillCircle(vBody, (int)(star.vPlanets[i].diameter * 1.0f), tmp);


				// if planet has rings, draw them here
				if (star.vPlanets[i].ring)
				{
					DrawLine(vBody.x - (star.vPlanets[i].diameter + 1), vBody.y, vBody.x + (star.vPlanets[i].diameter + 2), vBody.y, olc::DARK_GREY);
					DrawLine(vBody.x - (star.vPlanets[i].diameter + 1), vBody.y + 0.5f, vBody.x + (star.vPlanets[i].diameter + 2), vBody.y + 0.5f, olc::DARK_GREY);
				}

				olc::vi2d vMoon = vBody;
				vMoon.y += star.vPlanets[i].diameter + 10;

				// draw moons
				for (auto& moon : star.vPlanets[i].vMoons)
				{
					// check for clipping
					if (vMoon.y + (star.vPlanets[i].diameter + 10) >= 232 * 2)
					{
						break;
					}

					vMoon.y += moon;
					FillCircle(vMoon, (int)(moon * 1.0f), olc::GREY);
					vMoon.y += moon + 10;
				}

				vBody.x += star.vPlanets[i].diameter + 8;
			}
		}

		return true;
	}
};
#pragma endregion

// Main Program Loop
int main()
{
	Galaxy demo;

	// Construct Console Window at 512 x 480, and each 'game' pixel is 2 x 2
	if (demo.Construct(512, 480, 2, 2))
		demo.Start();

	return 0;
}