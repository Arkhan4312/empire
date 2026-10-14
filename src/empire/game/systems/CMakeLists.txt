# ArmySystem EconomySystem EraSystem ProductionSystem UpgradeSystem
empire_add_module(game_systems
	STATIC
	ALIAS empire::game::systems
	SOURCES
		ArmySystem.h
		ArmySystem.cpp
		EconomySystem.h
		EconomySystem.cpp
		EraSystem.h
		EraSystem.cpp
		ProductionSystem.h
		ProductionSystem.cpp
		UpgradeSystem.h
		UpgradeSystem.cpp
	PUBLIC
		empire::includes
		empire::game::state
		empire::game::content
)