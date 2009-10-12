//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_ITEMS_H__
#define __OTSERV_ITEMS_H__

#include "classes.h"
#include "const.h"
#include "enums.h"
#include "condition_attributes.h"
#include "itemloader.h"

enum ItemTypes_t {
	ITEM_TYPE_NONE = 0,
	ITEM_TYPE_DEPOT,
	ITEM_TYPE_MAILBOX,
	ITEM_TYPE_TRASHHOLDER,
	ITEM_TYPE_CONTAINER,
	ITEM_TYPE_DOOR,
	ITEM_TYPE_MAGICFIELD,
	ITEM_TYPE_TELEPORT,
	ITEM_TYPE_BED,
	ITEM_TYPE_KEY,
	ITEM_TYPE_LAST
};

struct Abilities{
	Abilities();

	struct Absorb {
			int16_t resistances[CombatType::size];

			bool any() const;
			bool reduce(CombatType type, int32_t& dmg) const;
			std::ostream& getDescription(std::ostream& os) const;
	protected:
			std::ostream& getDescription(std::ostream& os, bool& first, CombatType combat_type) const;
	} absorb;
	bool cure[ConditionId::size];

	//elemental damage
	CombatType elementType;
	int16_t elementDamage;

	//extra skill modifiers
	int32_t skills[SkillType::size];

	//stats modifiers
	int32_t stats[SkillType::size];
	int32_t statsPercent[SkillType::size];

	int32_t speed;
	bool manaShield;
	bool invisible;

	bool regeneration;
	uint32_t healthGain;
	uint32_t healthTicks;

	uint32_t manaGain;
	uint32_t manaTicks;

	MechanicType mechanicImmunities;

	bool preventItemLoss;
	bool preventSkillLoss;
};

class ItemType {
private:
	//It is private because calling it can cause unexpected results
	ItemType(const ItemType& it){};

public:
	ItemType();
	~ItemType();

	itemgroup_t group;
	ItemTypes_t type;

	bool isGroundTile() const {return (group == ITEM_GROUP_GROUND);}
	bool isContainer() const {return (group == ITEM_GROUP_CONTAINER);}
	bool isSplash() const {return (group == ITEM_GROUP_SPLASH);}
	bool isFluidContainer() const {return (group == ITEM_GROUP_FLUID);}

	bool isDoor() const {return (type == ITEM_TYPE_DOOR);}
	bool isMagicField() const {return (type == ITEM_TYPE_MAGICFIELD);}
	bool isTeleport() const {return (type == ITEM_TYPE_TELEPORT);}
	bool isKey() const {return (type == ITEM_TYPE_KEY);}
	bool isDepot() const {return (type == ITEM_TYPE_DEPOT);}
	bool isMailbox() const {return (type == ITEM_TYPE_MAILBOX);}
	bool isTrashHolder() const {return (type == ITEM_TYPE_TRASHHOLDER);}
	bool hasSubType() const {return (isFluidContainer() || isSplash() || stackable || charges != 0);}
	bool isRune() const {return clientCharges;}
	bool isBed() const {return type == ITEM_TYPE_BED;}

	uint16_t id;
	uint16_t clientId;

	std::string    name;
	std::string    article;
	std::string    pluralName;
	std::string    description;
	uint16_t       maxItems;
	float          weight;
	bool           showCount;
	WeaponType     weaponType;
	Weapon*        weaponInstance;
	AmmunitionType ammoType;
	ShootEffect    shootType;
	MagicEffect    magicEffect;
	int32_t        attack;
	int32_t        defense;
	int32_t        extraDefense;
	int32_t        armor;
	SlotPosition   slotPosition;
	SlotType       wieldPosition;
	uint16_t       speed;
	int32_t        decayTo;
	uint32_t       decayTime;
	bool           stopTime;
	RaceType       corpseType;

	uint16_t       maxTextLen;
	uint16_t       writeOnceItemId;
	int32_t        alwaysOnTopOrder;
	int32_t        rotateTo;
	int32_t        runeMagicLevel;
	int32_t        runeLevel;
	std::string    runeSpellName;

	WieldInformation wieldInfo;
	std::string    vocationString;
	uint32_t       minRequiredLevel;
	uint32_t       minRequiredMagicLevel;

	int32_t lightLevel;
	int32_t lightColor;
	
	Direction bedPartnerDirection;
	uint16_t maleSleeperID;
	uint16_t femaleSleeperID;
	uint16_t noSleeperID;

	bool blockSolid;
	bool blockProjectile;
	bool blockPathFind;
	bool allowPickupable;
	bool hasHeight;
	bool isVertical;
	bool isHorizontal;
	bool isHangable;
	bool clientCharges;
	bool lookThrough;
	bool pickupable;
	bool rotateable;
	bool stackable;
	bool useable;
	bool moveable;
	bool alwaysOnTop;
	bool canReadText;
	bool canWriteText;
	bool floorChangeDown;
	bool floorChangeNorth;
	bool floorChangeSouth;
	bool floorChangeEast;
	bool floorChangeWest;
	bool allowDistRead;

	unsigned short transformEquipTo;
	unsigned short transformDeEquipTo;
	bool showDuration;
	bool showCharges;
	uint32_t charges;
	int32_t breakChance;
	int32_t hitChance;
	int32_t maxHitChance;
	uint32_t shootRange;
	AmmunitionAction ammoAction;
	FluidType fluidSource;

	uint32_t currency;

	Abilities abilities;
	CombatType combatType;
	bool replaceable;
};

template<typename A>
class Array{
public:
	Array(uint32_t n);
	~Array();

	A getElement(uint32_t id);
	const A getElement(uint32_t id) const;
	void addElement(A a, uint32_t pos);

	uint32_t size() {return m_size;}

private:
	A* m_data;
	uint32_t m_size;
};



class Items{
public:
	Items();
	~Items();

	bool reload();
	void clear();

	int loadFromOtb(std::string);

	const ItemType& operator[](int32_t id) const {return getItemType(id);}
	const ItemType& getItemType(int32_t id) const;
	ItemType& getItemType(int32_t id);
	const ItemType& getItemIdByClientId(int32_t spriteId) const;

	int32_t getItemIdByName(const std::string& name);

	static uint32_t dwMajorVersion;
	static uint32_t dwMinorVersion;
	static uint32_t dwBuildNumber;

	bool loadFromXml(const std::string& datadir);

	void addItemType(ItemType* iType);

	const ItemType* getElement(uint32_t id) const {return items.getElement(id);}
	ItemType* getElement(uint32_t id) {return items.getElement(id);}
	uint32_t size() {return items.size();}

	std::map<uint32_t, ItemType*> currencyMap;

protected:
	void loadWeaponDefaults();

	typedef std::map<int32_t, int32_t> ReverseItemMap;
	ReverseItemMap reverseItemMap;

	Array<ItemType*> items;
	std::string m_datadir;
};



template<typename A>
inline Array<A>::Array(uint32_t n)
{
	m_data = (A*)malloc(sizeof(A)*n);
	memset(m_data, 0, sizeof(A)*n);
	m_size = n;
}

template<typename A>
inline Array<A>::~Array()
{
	free(m_data);
}

template<typename A>
inline A Array<A>::getElement(uint32_t id)
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
inline const A Array<A>::getElement(uint32_t id) const
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
inline void Array<A>::addElement(A a, uint32_t pos)
{
#define INCREMENT 5000
	if(pos >= m_size){
		m_data = (A*)realloc(m_data, sizeof(A)*(pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A)*(pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}
	m_data[pos] = a;
}


#endif
