#pragma once

#include <cstdint>

// F1 25 공식 패킷 스펙 기준 바이트 정렬 (1바이트 단위 패킹)
#pragma pack(push, 1)

/*	Type		| Description
*	uint8	| Unsigned 8-bit integer
*	int8		| Signed 8-bit integer
*	uint16	| Unsigned 16-bit integer
*	int16	| Signed 16-bit integer
*	uint32	| Unsigned 32-bit integer
*	float		| Floating point (32-bit)
*	double	| Double-precision floating point (64-bit)
*	uint64	| Unsigned 64-bit integer
*	char		| Character
*/

// F1 25 패킷 공통 헤더 구조체 (Codemasters 표준 사양 기반에 근거)
struct PacketHeader {
	uint16_t	m_packetFormat;						// 2025
	uint8_t		m_gameYear;							// Game year - last two digits e.g. 25
	uint8_t		m_gameMajorVersion;			// Game major version - "X.00"
	uint8_t		m_gameMinorVersion;			// Game minor version - "1.XX"
	uint8_t		m_packetVersion;					// Version of this packet type, all start from 1
	uint8_t		m_packetId;								// Identifier for the packet type, see below
	uint64_t	m_sessionUID;							// Unique identifier for the session
	float		m_sessionTime;						// Session timestamp
	uint32_t	m_frameIdentifier;					// Identifier for the frame the data was retrieved on
	// Overall identifier for the frame the data was retrieved
	// on, doesn't go back after flashbacks
	uint32_t	m_overallFrameIdentifier;
	uint8_t		m_playerCarIndex;					// Index of player's car in the array
	// Index of secondary player's car in the array (splitscreen)
	// 255 if no second player
	uint8_t		m_secondaryPlayerCarIndex;
};
/* Packet IDs (m_packetId)
* Packet Name			| Value | Description
* Motion					| 0			| Contains all motion data for player's car – only sent while player is in control
* Session					| 1			| Data about the session – track, time left
* Lap Data				| 2			| Data about all the lap times of cars in the session
* Event						| 3			| Various notable events that happen during a session
* Participants			| 4			| List of participants in the session, mostly relevant for multiplayer
* Car Setups				| 5			| Packet detailing car setups for cars in the race
* Car Telemetry		| 6			| Telemetry data for all cars
* Car Status				| 7			| Status data for all cars
* Final Classification	| 8			| Final classification confirmation at the end of a race
* Lobby Info				| 9			| Information about players in a multiplayer lobby
* Car Damage			| 10		| Damage status for all cars
* Session History		| 11		| Lap and tyre data for session
* Tyre Sets				| 12		| Extended tyre set data
* Motion Ex				| 13		| Extended motion data for player car
* Time Trial				| 14		| Time Trial specific data
* Lap Positions			| 15		| Lap positions on each lap so a chart can be constructed
*/

// Motion Packet 
// Frequency: Rate as specified in menus
// Size: 1349 bytes
// Version: 1
struct CarMotionData
{
	float		m_worldPositionX;			// World space X position - metres
	float		m_worldPositionY;			// World space Y position
	float		m_worldPositionZ;			// World space Z position
	float		m_worldVelocityX;			// Velocity in world space X – metres/s
	float		m_worldVelocityY;			// Velocity in world space Y
	float		m_worldVelocityZ;			// Velocity in world space Z
	int16_t		m_worldForwardDirX;		// World space forward X direction (normalised)
	int16_t		m_worldForwardDirY;		// World space forward Y direction (normalised)
	int16_t		m_worldForwardDirZ;		// World space forward Z direction (normalised)
	int16_t		m_worldRightDirX;			// World space right X direction (normalised)
	int16_t		m_worldRightDirY;			// World space right Y direction (normalised)
	int16_t		m_worldRightDirZ;			// World space right Z direction (normalised)
	float		m_gForceLateral;				// Lateral G-Force component
	float		m_gForceLongitudinal;	// Longitudinal G-Force component
	float		m_gForceVertical;			// Vertical G-Force component
	float		m_yaw;								// Yaw angle in radians
	float		m_pitch;							// Pitch angle in radians
	float		m_roll;								// Roll angle in radians
};

struct PacketMotionData
{
	PacketHeader m_header;							// Header
	CarMotionData m_carMotionData[22];		// Data for all cars on track
};

// Session Packet
// Frequency: 2 per second
// Size: 753 bytes
// Version: 1
struct MarshalZone
{
	float m_zoneStart;				// Fraction (0..1) of way through the lap the marshal zone starts
	int8_t m_zoneFlag;				// -1 = invalid/unknown, 0 = none, 1 = green, 2 = blue, 3 = yellow
};

struct WeatherForecastSample
{
	uint8_t		m_sessionType;						// 0 = unknown, see appendix
	uint8_t		m_timeOffset;							// Time in minutes the forecast is for
	// Weather - 0 = clear, 1 = light cloud, 2 = overcast
	// 3 = light rain, 4 = heavy rain, 5 = storm
	uint8_t		m_weather;
	int8_t		m_trackTemperature;				// Track temp. in degrees Celsius
	int8_t		m_trackTemperatureChange;	// Track temp. change – 0 = up, 1 = down, 2 = no change
	int8_t		m_airTemperature;					// Air temp. in degrees celsius
	int8_t		m_airTemperatureChange;		// Air temp. change – 0 = up, 1 = down, 2 = no change
	uint8_t		m_rainPercentage;					// Percentage chance of rain (0-100)
};

struct PacketSessionData
{
	PacketHeader						m_header;											// Header
	uint8_t									m_weather;											// Weather - 0 = clear, 1 = light cloud, 2 = overcast
	// 3 = light rain, 4 = heavy rain, 5 = storm
	int8_t									m_trackTemperature;							// Track temp. in degrees celsius
	int8_t									m_airTemperature;								// Air temp. in degrees celsius
	uint8_t									m_totalLaps;										// Total number of laps in this race
	uint16_t								m_trackLength;									// Track length in metres
	uint8_t									m_sessionType;									// 0 = unknown, see appendix
	int8_t									m_trackId;											// -1 for unknown, see appendix
	// Formula, 0 = F1 Modern, 1 = F1 Classic, 2 = F2,
	// 3 = F1 Generic, 4 = Beta, 6 = Esports
	// 8 = F1 World, 9 = F1 Elimination
	uint8_t									m_formula;
	uint16_t								m_sessionTimeLeft;								// Time left in session in seconds
	uint16_t								m_sessionDuration;								// Session duration in seconds
	uint8_t									m_pitSpeedLimit;									// Pit speed limit in kilometres per hour
	uint8_t									m_gamePaused;									// Whether the game is paused – network game only
	uint8_t									m_isSpectating;									// Whether the player is spectating
	uint8_t									m_spectatorCarIndex;							// Index of the car being spectated
	uint8_t									m_sliProNativeSupport;						// SLI Pro support, 0 = inactive, 1 = active
	uint8_t									m_numMarshalZones;							// Number of marshal zones to follow
	MarshalZone						m_marshalZones[21];							// List of marshal zones – max 21
	// 0 = no safety car, 1 = full
	// 2 = virtual, 3 = formation lap
	uint8_t									m_safetyCarStatus;
	uint8_t									m_networkGame;								// 0 = offline, 1 = online
	uint8_t									m_numWeatherForecastSamples;		// Number of weather samples to follow
	WeatherForecastSample	m_weatherForecastSamples[64];		// Array of weather forecast samples
	uint8_t									m_forecastAccuracy;							// 0 = Perfect, 1 = Approximate
	uint8_t									m_aiDifficulty;										// AI Difficulty rating – 0-110
	uint32_t								m_seasonLinkIdentifier;						// Identifier for season - persists across saves
	uint32_t								m_weekendLinkIdentifier;					// Identifier for weekend - persists across saves
	uint32_t								m_sessionLinkIdentifier;						// Identifier for session - persists across saves
	uint8_t									m_pitStopWindowIdealLap;				// Ideal lap to pit on for current strategy (player)
	uint8_t									m_pitStopWindowLatestLap;				// Latest lap to pit on for current strategy (player)
	uint8_t									m_pitStopRejoinPosition;					// Predicted position to rejoin at (player)
	uint8_t									m_steeringAssist;									// 0 = off, 1 = on
	uint8_t									m_brakingAssist;									// 0 = off, 1 = low, 2 = medium, 3 = high
	uint8_t									m_gearboxAssist;									// 1 = manual, 2 = manual & suggested gear, 3 = auto
	uint8_t									m_pitAssist;											// 0 = off, 1 = on
	uint8_t									m_pitReleaseAssist;								// 0 = off, 1 = on
	uint8_t									m_ERSAssist;										// 0 = off, 1 = on
	uint8_t									m_DRSAssist;										// 0 = off, 1 = on
	uint8_t									m_dynamicRacingLine;						// 0 = off, 1 = corners only, 2 = full
	uint8_t									m_dynamicRacingLineType;				// 0 = 2D, 1 = 3D
	uint8_t									m_gameMode;									// Game mode id - see appendix
	uint8_t									m_ruleSet;											// Ruleset - see appendix
	uint32_t								m_timeOfDay;										// Local time of day - minutes since midnight
	// 0 = None, 2 = Very Short, 3 = Short, 4 = Medium
	// 5 = Medium Long, 6 = Long, 7 = Full
	uint8_t									m_sessionLength;
	uint8_t									m_speedUnitsLeadPlayer;					// 0 = MPH, 1 = KPH
	uint8_t									m_temperatureUnitsLeadPlayer;			// 0 = Celsius, 1 = Fahrenheit
	uint8_t									m_speedUnitsSecondaryPlayer;			// 0 = MPH, 1 = KPH
	uint8_t									m_temperatureUnitsSecondaryPlayer;// 0 = Celsius, 1 = Fahrenheit
	uint8_t									m_numSafetyCarPeriods;					// Number of safety cars called during session
	uint8_t									m_numVirtualSafetyCarPeriods;			// Number of virtual safety cars called
	uint8_t									m_numRedFlagPeriods;						// Number of red flags called during session
	uint8_t									m_equalCarPerformance;					// 0 = Off, 1 = On
	uint8_t									m_recoveryMode;								// 0 = None, 1 = Flashbacks, 2 = Auto-recovery
	uint8_t									m_flashbackLimit;								// 0 = Low, 1 = Medium, 2 = High, 3 = Unlimited
	uint8_t									m_surfaceType;									// 0 = Simplified, 1 = Realistic
	uint8_t									m_lowFuelMode;									// 0 = Easy, 1 = Hard
	uint8_t									m_raceStarts;										// 0 = Manual, 1 = Assisted
	uint8_t									m_tyreTemperature;							// 0 = Surface only, 1 = Surface & Carcass
	uint8_t									m_pitLaneTyreSim;								// 0 = On, 1 = Off
	uint8_t									m_carDamage;										// 0 = Off, 1 = Reduced, 2 = Standard, 3 = Simulation
	uint8_t									m_carDamageRate;								// 0 = Reduced, 1 = Standard, 2 = Simulation
	uint8_t									m_collisions;										// 0 = Off, 1 = Player-to-Player Off, 2 = On
	uint8_t									m_collisionsOffForFirstLapOnly;			// 0 = Disabled, 1 = Enabled
	uint8_t									m_mpUnsafePitRelease;						// 0 = On, 1 = Off (Multiplayer)
	uint8_t									m_mpOffForGriefing;							// 0 = Disabled, 1 = Enabled (Multiplayer)
	uint8_t									m_cornerCuttingStringency;				// 0 = Regular, 1 = Strict
	uint8_t									m_parcFermeRules;								// 0 = Off, 1 = On
	uint8_t									m_pitStopExperience;							// 0 = Automatic, 1 = Broadcast, 2 = Immersive
	uint8_t									m_safetyCar;										// 0 = Off, 1 = Reduced, 2 = Standard, 3 = Increased
	uint8_t									m_safetyCarExperience;						// 0 = Broadcast, 1 = Immersive
	uint8_t									m_formationLap;									// 0 = Off, 1 = On
	uint8_t									m_formationLapExperience;				// 0 = Broadcast, 1 = Immersive
	uint8_t									m_redFlags;											// 0 = Off, 1 = Reduced, 2 = Standard, 3 = Increased
	uint8_t									m_affectsLicenceLevelSolo;					// 0 = Off, 1 = On
	uint8_t									m_affectsLicenceLevelMP;					// 0 = Off, 1 = On
	uint8_t									m_numSessionsInWeekend;				// Number of session in following array
	// List of session types to show weekend
	// structure - see appendix for types
	uint8_t									m_weekendStructure[12];
	float									m_sector2LapDistanceStart;				// Distance in m around track where sector 2 starts
	float									m_sector3LapDistanceStart;				// Distance in m around track where sector 3 starts
};

// Lap Data Packet
// Frequency: Rate as specified in menus
// Size: 1285 bytes
// Version: 1
struct LapData
{
	uint32_t		m_lastLapTimeInMS;								// Last lap time in milliseconds
	uint32_t		m_currentLapTimeInMS;							// Current time around the lap in milliseconds
	uint16_t		m_sector1TimeMSPart;							// Sector 1 time milliseconds part
	uint8_t			m_sector1TimeMinutesPart;					// Sector 1 whole minute part
	uint16_t		m_sector2TimeMSPart;							// Sector 2 time milliseconds part
	uint8_t			m_sector2TimeMinutesPart;					// Sector 2 whole minute part
	uint16_t		m_deltaToCarInFrontMSPart;					// Time delta to car in front milliseconds part
	uint8_t			m_deltaToCarInFrontMinutesPart;			// Time delta to car in front whole minute part
	uint16_t		m_deltaToRaceLeaderMSPart;					// Time delta to race leader milliseconds part
	uint8_t			m_deltaToRaceLeaderMinutesPart;			// Time delta to race leader whole minute part
	// Distance vehicle is around current lap in metres – could
	// be negative if line hasn’t been crossed yet
	float			m_lapDistance;
	// Total distance travelled in session in metres – could
	// be negative if line hasn’t been crossed yet
	float			m_totalDistance;
	float			m_safetyCarDelta;									// Delta in seconds for safety car
	uint8_t			m_carPosition;											// Car race position
	uint8_t			m_currentLapNum;									// Current lap number
	uint8_t			m_pitStatus;												// 0 = none, 1 = pitting, 2 = in pit area
	uint8_t			m_numPitStops;										// Number of pit stops taken in this race
	uint8_t			m_sector;													// 0 = sector1, 1 = sector2, 2 = sector3
	uint8_t			m_currentLapInvalid;								// Current lap invalid - 0 = valid, 1 = invalid
	uint8_t			m_penalties;											// Accumulated time penalties in seconds to be added
	uint8_t			m_totalWarnings;									// Accumulated number of warnings issued
	uint8_t			m_cornerCuttingWarnings;						// Accumulated number of corner cutting warnings issued
	uint8_t			m_numUnservedDriveThroughPens;		// Num drive through pens left to serve
	uint8_t			m_numUnservedStopGoPens;					// Num stop go pens left to serve
	uint8_t			m_gridPosition;										// Grid position the vehicle started the race in
	// Status of driver - 0 = in garage, 1 = flying lap
	// 2 = in lap, 3 = out lap, 4 = on track
	uint8_t			m_driverStatus;
	// Result status - 0 = invalid, 1 = inactive, 2 = active
	// 3 = finished, 4 = didnotfinish, 5 = disqualified
	// 6 = not classified, 7 = retired
	uint8_t			m_resultStatus;
	uint8_t			m_pitLaneTimerActive;							// Pit lane timing, 0 = inactive, 1 = active
	uint16_t		m_pitLaneTimeInLaneInMS;					// If active, the current time spent in the pit lane in ms
	uint16_t		m_pitStopTimerInMS;								// Time of the actual pit stop in ms
	uint8_t			m_pitStopShouldServePen;						// Whether the car should serve a penalty at this stop
	float			m_speedTrapFastestSpeed;						// Fastest speed through speed trap for this car in kmph
	uint8_t			m_speedTrapFastestLap;							// Lap no the fastest speed was achieved, 255 = not set
};

struct PacketLapData
{
	PacketHeader		m_header;										// Header
	LapData				m_lapData[22];								// Lap data for all cars on track
	uint8_t					m_timeTrialPBCarIdx;						// Index of Personal Best car in time trial (255 if invalid)
	uint8_t					m_timeTrialRivalCarIdx;					// Index of Rival car in time trial (255 if invalid)
};


// Event Packet
// Frequency: When the event occurs
// Size: 45 bytes
// Version: 1

// The event details packet is different for each type of event.
// Make sure only the correct type is interpreted.
union EventDataDetails
{
	struct
	{
		uint8_t		vehicleIdx;		// Vehicle index of car achieving fastest lap
		float		lapTime;			// Lap time is in seconds
	} FastestLap;

	struct
	{
		uint8_t		vehicleIdx;		// Vehicle index of car retiring
		// Reason - 0 = invalid, 1 = retired, 2 = finished
		// 3 = terminal damage, 4 = inactive, 5 = not enough laps completed
		// 6 = black flagged, 7 = red flagged, 8 = mechanical failure
		// 9 = session skipped, 10 = session simulated
		uint8_t		reason;
	} Retirement;

	struct
	{
		// 0 = Wet track, 1 = Safety car deployed, 2 = Red flag
		// 3 = Min lap not reached
		uint8_t		reason;
	} DRSDisabled;

	struct
	{
		uint8_t		vehicleIdx;		// Vehicle index of team mate
	} TeamMateInPits;

	struct
	{
		uint8_t		vehicleIdx;		// Vehicle index of the race winner
	} RaceWinner;

	struct
	{
		uint8_t		penaltyType;			// Penalty type – see Appendices
		uint8_t		infringementType;	// Infringement type – see Appendices
		uint8_t		vehicleIdx;				// Vehicle index of the car the penalty is applied to
		uint8_t		otherVehicleIdx;		// Vehicle index of the other car involved
		uint8_t		time;						// Time gained, or time spent doing action in seconds
		uint8_t		lapNum;					// Lap the penalty occurred on
		uint8_t		placesGained;			// Number of places gained by this
	} Penalty;

	struct
	{
		uint8_t		vehicleIdx;								// Vehicle index of the vehicle triggering speed trap
		float		speed;										// Top speed achieved in kilometres per hour
		uint8_t		isOverallFastestInSession;		// Overall fastest speed in session = 1, otherwise 0
		uint8_t		isDriverFastestInSession;			// Fastest speed for driver in session = 1, otherwise 0
		uint8_t		fastestVehicleIdxInSession;		// Vehicle index of the vehicle that is the fastest
		// in this session
		float		fastestSpeedInSession;			// Speed of the vehicle that is the fastest
		// in this session
	} SpeedTrap;

	struct
	{
		uint8_t		numLights;								// Number of lights showing
	} StartLIghts;

	struct
	{
		uint8_t		vehicleIdx;								// Vehicle index of the vehicle serving drive through
	} DriveThroughPenaltyServed;

	struct
	{
		uint8_t		vehicleIdx;								// Vehicle index of the vehicle serving stop go
		float		stopTime;									// Time spent serving stop go in seconds
	} StopGoPenaltyServed;

	struct
	{
		uint32_t	flashbackFrameIdentifier;		// Frame identifier flashed back to
		float		flashbackSessionTime;				// Session time flashed back to
	} Flashback;

	struct
	{
		// Bit flags specifying which buttons are being pressed
		// currently - see appendices
		uint32_t	buttonStatus;
	} Buttons;

	struct
	{
		uint8_t		overtakingVehicleIdx;				// Vehicle index of the vehicle overtaking
		uint8_t		beingOvertakenVehicleIdx;		// Vehicle index of the vehicle being overtaken
	} Overtake;

	struct
	{
		// 0 = No Safety Car, 1 = Full Safety Car
		// 2 = Virtual Safety Car, 3 = Formation Lap Safety Car
		uint8_t		safetyCarType;
		// 0 = Deployed, 1 = Returning, 2 = Returned
		// 3 = Resume Race
		uint8_t		eventType;
	} SafetyCar;

	struct
	{
		uint8_t		vehicle1Idx;								// Vehicle index of the first vehicle involved in the collision
		uint8_t		vehicle2Idx;								// Vehicle index of the second vehicle involved in the collision
	} Collision;
};

struct PacketEventData
{
	PacketHeader			m_header;						// Header

	// Event string code, see below
	//	Event							| Code		| Description
	//	Session Started			|“SSTA”| Sent when the session starts
	//	Session Ended				|“SEND”| Sent when the session ends
	//	Fastest Lap					|“FTLP”	| When a driver achieves the fastest lap
	//	Retirement					|“RTMT”| When a driver retires
	//	DRS enabled				|“DRSE”| Race control have enabled DRS
	//	DRS disabled				|“DRSD”| Race control have disabled DRS
	//	Team mate in pits		|“TMPT”| Your team mate has entered the pits
	//	Chequered flag			|“CHQF”| The chequered flag has been waved
	//	Race Winner				|“RCWN”| The race winner is announced
	//	Penalty Issued				|“PENA”| A penalty has been issued – details in event
	//	Speed Trap Triggered	|“SPTP”| Speed trap has been triggered by fastest speed
	//	Start lights					|“STLG”| Start lights – number shown
	//	Lights out					|“LGOT”| Lights out
	//	Drive through served	|“DTSV”| Drive through penalty served
	//	Stop go served			|“SGSV”| Stop go penalty served
	//	Flashback						|“FLBK”| Flashback activated
	//	Button status				|“BUTN”| Button status changed
	//	Red Flag						|“RDFL”| Red flag shown
	//	Overtake						|“OVTK”| Overtake occurred
	//	Safety Car					|“SCAR”| Safety car event – details in event
	//	Collision						|“COLL”| Collision between two vehicles has occurred
	uint8_t						m_eventStringCode[4];
	// Event details - should be interpreted differently
	// for each type
	EventDataDetails	m_eventDetails;
};


// Participants Packet
// Frequency: Every 5 seconds
// Size: 1284 bytes
// Version: 1
// RGB value of a colour
struct LiveryColour
{
	uint8_t				red;
	uint8_t				green;
	uint8_t				blue;
};
struct ParticipantData
{
	uint8_t				m_aiControlled;			// Whether the vehicle is AI (1) or Human (0) controlled
	uint8_t				m_driverId;					// Driver id - see appendix, 255 if network human
	uint8_t				m_networkId;				// Network id – unique identifier for network players
	uint8_t				m_teamId;					// Team id - see appendix
	uint8_t				m_myTeam;					// My team flag – 1 = My Team, 0 = otherwise
	uint8_t				m_raceNumber;			// Race number of the car
	uint8_t				m_nationality;				// Nationality of the driver
	// Name of participant in UTF-8 format – null terminated
	// Will be truncated with … (U+2026) if too long
	char					m_name[32];
	uint8_t				m_yourTelemetry;		// The player's UDP setting, 0 = restricted, 1 = public
	uint8_t				m_showOnlineNames;	// The player's show online names setting, 0 = off, 1 = on
	uint16_t			m_techLevel;				// F1 World tech level
	uint8_t				m_platform;					// 1 = Steam, 3 = PlayStation, 4 = Xbox, 6 = Origin, 255 = unknown
	uint8_t				m_numColours;			// Number of colours valid for this car
	LiveryColour	m_liveryColours[4];		// Colours for the car
};

struct PacketParticipantsData
{
	PacketHeader	m_header;					// Header
	uint8_t				m_numActiveCars;		// Number of active cars in the data – should match number of
	// cars on HUD
	ParticipantData m_participants[22];
};

//Car Setups Packet
// Frequency: 2 per second
// Size: 1133 bytes
// Version: 1
struct CarSetupData
{
	uint8_t		m_frontWing;							// Front wing aero
	uint8_t		m_rearWing;							// Rear wing aero
	uint8_t		m_onThrottle;							// Differential adjustment on throttle (percentage)
	uint8_t		m_offThrottle;							// Differential adjustment off throttle (percentage)
	float		m_frontCamber;						// Front camber angle (suspension geometry)
	float		m_rearCamber;						// Rear camber angle (suspension geometry)
	float		m_frontToe;								// Front toe angle (suspension geometry)
	float		m_rearToe;								// Rear toe angle (suspension geometry)
	uint8_t		m_frontSuspension;					// Front suspension
	uint8_t		m_rearSuspension;					// Rear suspension
	uint8_t		m_frontAntiRollBar;					// Front anti-roll bar
	uint8_t		m_rearAntiRollBar;					// Front anti-roll bar
	uint8_t		m_frontSuspensionHeight;		// Front ride height
	uint8_t		m_rearSuspensionHeight;		// Rear ride height
	uint8_t		m_brakePressure;					// Brake pressure (percentage)
	uint8_t		m_brakeBias;							// Brake bias (percentage)
	uint8_t		m_engineBraking;					// Engine braking (percentage)
	float		m_rearLeftTyrePressure;			// Rear left tyre pressure (PSI)
	float		m_rearRightTyrePressure;		// Rear right tyre pressure (PSI)
	float		m_frontLeftTyrePressure;			// Front left tyre pressure (PSI)
	float		m_frontRightTyrePressure;		// Front right tyre pressure (PSI)
	uint8_t		m_ballast;								// Ballast
	float		m_fuelLoad;								// Fuel load
};

struct PacketCarSetupData
{
	PacketHeader		m_header;						// Header
	CarSetupData		m_carSetups[22];
	float					m_nextFrontWingValue;	// Value of front wing after next pit stop - player only
};


// Car Telemetry Packet
// Frequency: Rate as specified in menus
// Size: 1352 bytes
// Version: 1
struct CarTelemetryData
{
	uint16_t	m_speed;											// Speed of car in kilometres per hour
	float		m_throttle;										// Amount of throttle applied (0.0 to 1.0)
	float		m_steer;											// Steering (-1.0 (full lock left) to 1.0 (full lock right))
	float		m_brake;											// Amount of brake applied (0.0 to 1.0)
	uint8_t		m_clutch;											// Amount of clutch applied (0 to 100)
	int8_t		m_gear;											// Gear selected (1-8, N=0, R=-1)
	uint16_t	m_engineRPM;								// Engine RPM
	uint8_t		m_drs;												// 0 = off, 1 = on
	uint8_t		m_revLightsPercent;						// Rev lights indicator (percentage)
	uint16_t	m_revLightsBitValue;						// Rev lights (bit 0 = leftmost LED, bit 14 = rightmost LED)
	uint16_t	m_brakesTemperature[4];				// Brakes temperature (celsius)
	uint8_t		m_tyresSurfaceTemperature[4];		// Tyres surface temperature (celsius)
	uint8_t		m_tyresInnerTemperature[4];			// Tyres inner temperature (celsius)
	uint16_t	m_engineTemperature;					// Engine temperature (celsius)
	float		m_tyresPressure[4];							// Tyres pressure (PSI)
	uint8_t		m_surfaceType[4];							// Driving surface, see appendices
};
struct PacketCarTelemetryData
{
	PacketHeader				m_header;							// Header
	CarTelemetryData		m_carTelemetryData[22];
	// Index of MFD panel open - 255 = MFD closed
	// Single player, race – 0 = Car setup, 1 = Pits
	// 2 = Damage, 3 = Engine, 4 = Temperatures
	// May vary depending on game mode
	uint8_t							m_mfdPanelIndex;
	uint8_t							m_mfdPanelIndexSecondaryPlayer;	// See above
	// Suggested gear for the player (1-8)
	// 0 if no gear suggested
	int8_t							m_suggestedGear;
};


// Car Status Packet
// Frequency: Rate as specified in menus
// Size: 1239 bytes
// Version: 1
struct CarStatusData
{
	uint8_t		m_tractionControl;						// Traction control - 0 = off, 1 = medium, 2 = full
	uint8_t		m_antiLockBrakes;						// 0 (off) - 1 (on)
	uint8_t		m_fuelMix;									// Fuel mix - 0 = lean, 1 = standard, 2 = rich, 3 = max
	uint8_t		m_frontBrakeBias;						// Front brake bias (percentage)
	uint8_t		m_pitLimiterStatus;						// Pit limiter status - 0 = off, 1 = on
	float		m_fuelInTank;								// Current fuel mass
	float		m_fuelCapacity;							// Fuel capacity
	float		m_fuelRemainingLaps;				// Fuel remaining in terms of laps (value on MFD)
	uint16_t	m_maxRPM;									// Cars max RPM, point of rev limiter
	uint16_t	m_idleRPM;									// Cars idle RPM
	uint8_t		m_maxGears;								// Maximum number of gears
	// 0 = not allowed, 1 = allowed
	// 0 = DRS not available, non-zero - DRS will be available
	// in [X] metres
	uint8_t		m_drsAllowed;
	uint16_t	m_drsActivationDistance;
	// F1 Modern - 16 = C5, 17 = C4, 18 = C3, 19 = C2, 20 = C1
	// 21 = C0, 22 = C6, 7 = inter, 8 = wet
	// F1 Classic - 9 = dry, 10 = wet
	// F2 – 11 = super soft, 12 = soft, 13 = medium, 14 = hard
	// 15 = wet
	uint8_t		m_actualTyreCompound;
	// F1 visual (can be different from actual compound)
	// 16 = soft, 17 = medium, 18 = hard, 7 = inter, 8 = wet
	// F1 Classic – same as above
	// F2 ‘20, 15 = wet, 19 – super soft, 20 = soft
	// 21 = medium, 22 = hard
	uint8_t		m_visualTyreCompound;
	uint8_t		m_tyresAgeLaps;							// Age in laps of the current set of tyres
	// -1 = invalid/unknown, 0 = none, 1 = green
	// 2 = blue, 3 = yellow
	int8_t		m_vehicleFiaFlags;
	float		m_enginePowerICE;						// Engine power output of ICE (W)
	float		m_enginePowerMGUK;				// Engine power output of MGU-K (W)
	float		m_ersStoreEnergy;						// ERS energy store in Joules
	// ERS deployment mode, 0 = none, 1 = medium
	// 2 = hotlap, 3 = overtake
	uint8_t		m_ersDeployMode;
	float		m_ersHarvestedThisLapMGUK;	// ERS energy harvested this lap by MGU-K
	float		m_ersHarvestedThisLapMGUH;	// ERS energy harvested this lap by MGU-H
	float		m_ersDeployedThisLap;				// ERS energy deployed this lap
	uint8_t		m_networkPaused;						// Whether the car is paused in a network game
};
struct PacketCarStatusData
{
	PacketHeader		m_header; // Header
	CarStatusData	m_carStatusData[22];
};


// Final Classification Packet
// Frequency: Once at the end of a race
// Size: 1042 bytes
// Version: 1

struct FinalClassificationData
{
	uint8_t		m_position;							// Finishing position
	uint8_t		m_numLaps;						// Number of laps completed
	uint8_t		m_gridPosition;					// Grid position of the car
	uint8_t		m_points;								// Number of points scored
	uint8_t		m_numPitStops;					// Number of pit stops made
	// Result status - 0 = invalid, 1 = inactive, 2 = active
	// 3 = finished, 4 = didnotfinish, 5 = disqualified
	// 6 = not classified, 7 = retired
	uint8_t		m_resultStatus;
	// Result reason - 0 = invalid, 1 = retired, 2 = finished
	// 3 = terminal damage, 4 = inactive, 5 = not enough laps completed
   // 6 = black flagged, 7 = red flagged, 8 = mechanical failure
   // 9 = session skipped, 10 = session simulated
	uint8_t		m_resultReason;
	uint32_t	m_bestLapTimeInMS;			// Best lap time of the session in milliseconds
	double		m_totalRaceTime;				// Total race time in seconds without penalties
	uint8_t		m_penaltiesTime;					// Total penalties accumulated in seconds
	uint8_t		m_numPenalties;					// Number of penalties applied to this driver
	uint8_t		m_numTyreStints;				// Number of tyres stints up to maximum
	uint8_t		m_tyreStintsActual[8];			// Actual tyres used by this driver
	uint8_t		m_tyreStintsVisual[8];			// Visual tyres used by this driver
	uint8_t		m_tyreStintsEndLaps[8];		// The lap number stints end on
};
struct PacketFinalClassificationData
{
	PacketHeader					m_header;		// Header
	uint8_t								m_numCars;		// Number of cars in the final classification
	FinalClassificationData m_classificationData[22];
};


// Lobby Info Packet
// Frequency: Two every second when in the lobby
// Size: 954 bytes
// Version: 1

struct LobbyInfoData
{
	uint8_t		m_aiControlled;				// Whether the vehicle is AI (1) or Human (0) controlled
	uint8_t		m_teamId;						// Team id - see appendix (255 if no team currently selected)
	uint8_t		m_nationality;					// Nationality of the driver
	uint8_t		m_platform;						// 1 = Steam, 3 = PlayStation, 4 = Xbox, 6 = Origin, 255 = unknown
	// Name of participant in UTF-8 format – null terminated
	// Will be truncated with ... (U+2026) if too long
	char			m_name[32];
	uint8_t		m_carNumber;					// Car number of the player
	uint8_t		m_yourTelemetry;			// The player's UDP setting, 0 = restricted, 1 = public
	uint8_t		m_showOnlineNames;		// The player's show online names setting, 0 = off, 1 = on
	uint16_t	m_techLevel;					// F1 World tech level
	uint8_t		m_readyStatus;				// 0 = not ready, 1 = ready, 2 = spectating
};
struct PacketLobbyInfoData
{
	PacketHeader		m_header;					// Header
	// Packet specific data
	uint8_t					m_numPlayers;			// Number of players in the lobby data
	LobbyInfoData	m_lobbyPlayers[22];
};


// Session History Packet
// Frequency: 20 per second but cycling through cars
// Size: 1460 bytes
// Version: 1

struct LapHistoryData
{
	uint32_t							m_lapTimeInMS;						// Lap time in milliseconds
	uint16_t							m_sector1TimeMSPart;			// Sector 1 milliseconds part
	uint8_t								m_sector1TimeMinutesPart;	// Sector 1 whole minute part
	uint16_t							m_sector2TimeMSPart;			// Sector 2 time milliseconds part
	uint8_t								m_sector2TimeMinutesPart;	// Sector 2 whole minute part
	uint16_t							m_sector3TimeMSPart;			// Sector 3 time milliseconds part
	uint8_t								m_sector3TimeMinutesPart;	// Sector 3 whole minute part
	// 0x01 bit set-lap valid, 0x02 bit set-sector 1 valid
	// 0x04 bit set-sector 2 valid, 0x08 bit set-sector 3 valid
	uint8_t								m_lapValidBitFlags;
};
struct TyreStintHistoryData
{
	uint8_t								m_endLap; // Lap the tyre usage ends on (255 of current tyre)
	uint8_t								m_tyreActualCompound; // Actual tyres used by this driver
	uint8_t								m_tyreVisualCompound; // Visual tyres used by this driver
};
struct PacketSessionHistoryData
{
	PacketHeader					m_header; // Header
	uint8_t								m_carIdx; // Index of the car this lap data relates to
	uint8_t								m_numLaps; // Num laps in the data (including current partial lap)
	uint8_t								m_numTyreStints; // Number of tyre stints in the data
	uint8_t								m_bestLapTimeLapNum; // Lap the best lap time was achieved on
	uint8_t								m_bestSector1LapNum; // Lap the best Sector 1 time was achieved on
	uint8_t								m_bestSector2LapNum; // Lap the best Sector 2 time was achieved on
	uint8_t								m_bestSector3LapNum; // Lap the best Sector 3 time was achieved on
	LapHistoryData				m_lapHistoryData[100]; // 100 laps of data max
	TyreStintHistoryData	m_tyreStintsHistoryData[8];
};


// Tyre Sets Packet
// Frequency: 20 per second but cycling through cars
// Size: 231 bytes
// Version: 1

struct TyreSetData
{
	uint8_t						m_actualTyreCompound;	// Actual tyre compound used
	uint8_t						m_visualTyreCompound;		// Visual tyre compound used
	uint8_t						m_wear;								// Tyre wear (percentage)
	uint8_t						m_available;							// Whether this set is currently available
	uint8_t						m_recommendedSession;	// Recommended session for tyre set, see appendix
	uint8_t						m_lifeSpan;							// Laps left in this tyre set
	uint8_t						m_usableLife;						// Max number of laps recommended for this compound
	int16_t						m_lapDeltaTime;					// Lap delta time in milliseconds compared to fitted set
	uint8_t						m_fitted;								// Whether the set is fitted or not
};
struct PacketTyreSetsData
{
	PacketHeader			m_header;							// Header
	uint8_t						m_carIdx;								// Index of the car this data relates to
	TyreSetData			m_tyreSetData[20];				// 13 (dry) + 7 (wet)
	uint8_t						m_fittedIdx;							// Index into array of fitted tyre
};


// Motion Ex Packet
// Frequency: Rate as specified in menus
// Size: 273 bytes
// Version: 1

struct PacketMotionExData
{
	PacketHeader m_header; // Header
	// Extra player car ONLY data
	float				m_suspensionPosition[4];			// Note: All wheel arrays have the following order:
	float				m_suspensionVelocity[4];			// RL, RR, FL, FR
	float				m_suspensionAcceleration[4];	// RL, RR, FL, FR
	float				m_wheelSpeed[4];						// Speed of each wheel
	float				m_wheelSlipRatio[4];					// Slip ratio for each wheel
	float				m_wheelSlipAngle[4];					// Slip angles for each wheel
	float				m_wheelLatForce[4];					// Lateral forces for each wheel
	float				m_wheelLongForce[4];				// Longitudinal forces for each wheel
	float				m_heightOfCOGAboveGround;	// Height of centre of gravity above ground
	float				m_localVelocityX;						// Velocity in local space – metres/s
	float				m_localVelocityY;							// Velocity in local space
	float				m_localVelocityZ;							// Velocity in local space
	float				m_angularVelocityX;					// Angular velocity x-component – radians/s
	float				m_angularVelocityY;					// Angular velocity y-component
	float				m_angularVelocityZ;					// Angular velocity z-component
	float				m_angularAccelerationX;				// Angular acceleration x-component – radians/s/s
	float				m_angularAccelerationY;				// Angular acceleration y-component
	float				m_angularAccelerationZ;				// Angular acceleration z-component
	float				m_frontWheelsAngle;					// Current front wheels angle in radians
	float				m_wheelVertForce[4];					// Vertical forces for each wheel
	float				m_frontAeroHeight;					// Front plank edge height above road surface
	float				m_rearAeroHeight;						// Rear plank edge height above road surface
	float				m_frontRollAngle;						// Roll angle of the front suspension
	float				m_rearRollAngle;							// Roll angle of the rear suspension
	// Yaw angle of the chassis relative to the direction
	// of motion - radians
	float				m_chassisYaw;
	// Pitch angle of the chassis relative to the
	// direction of motion – radians
	float				m_chassisPitch;
	float				m_wheelCamber[4];					// Camber of each wheel in radians
	// Camber gain for each wheel in radians, difference
	// between active camber and dynamic camber
	float				m_wheelCamberGain[4];
};

// Time Trial Packet
// Frequency: 1 per second
// Size: 101 bytes
// Version: 1

struct TimeTrialDataSet
{
	uint8_t						m_carIdx;								// Index of the car this data relates to
	uint8_t						m_teamId;							// Team id - see appendix
	uint32_t					m_lapTimeInMS;					// Lap time in milliseconds
	uint32_t					m_sector1TimeInMS;			// Sector 1 time in milliseconds
	uint32_t					m_sector2TimeInMS;			// Sector 2 time in milliseconds
	uint32_t					m_sector3TimeInMS;			// Sector 3 time in milliseconds
	uint8_t						m_tractionControl;				// 0 = assist off, 1 = assist on
	uint8_t						m_gearboxAssist;					// 0 = assist off, 1 = assist on
	uint8_t						m_antiLockBrakes;				// 0 = assist off, 1 = assist on
	uint8_t						m_equalCarPerformance;	// 0 = Realistic, 1 = Equal
	uint8_t						m_customSetup;					// 0 = No, 1 = Yes
	uint8_t						m_valid;								// 0 = invalid, 1 = valid
};
struct PacketTimeTrialData
{
	PacketHeader			m_header;									// Header
	TimeTrialDataSet	m_playerSessionBestDataSet;		// Player session best data set
	TimeTrialDataSet	m_personalBestDataSet;				// Personal best data set
	TimeTrialDataSet	m_rivalDataSet;							// Rival data set
};


// Lap Positions Packet
// Frequency: 1 per second
// Size: 1131 bytes
// Version: 1

struct PacketLapPositionsData
{
	PacketHeader		m_header; // Header
	// Packet specific data
	uint8_t					m_numLaps; // Number of laps in the data
	uint8_t					m_lapStart; // Index of the lap where the data starts, 0 indexed
	// Array holding the position of the car in a given lap, 0 if no record
	uint8_t					m_positionForVehicleIdx[50];		// [cs_maxNumCarsInUDPData] ;
};


// 검증용 단순화 텔레메트리 데이터 구조체
struct MotionTelemetryData {
	float tyrePressure[4];									// 타이어 압력 (FL, FR, RL, RR)
	float suspensionPosition[4];						// 서스펜션 위치 
	float speed;													// 차량 속도
};

// 소비자 스레드에서 화면에 노출할 스레드 안전 공유 변수 (디버그 프리뷰용)
struct LiveDisplayMetrics {
	uint32_t		frameId = 0;
	float			sessionTime = 0.0f;
	uint16_t		speed = 0;
	int8_t			gear = 0;
	uint16_t		rpm = 0;
	float			tyrePressure[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

#pragma pack(pop)
