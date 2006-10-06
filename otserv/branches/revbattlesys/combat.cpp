//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#include "combat.h"

#include "game.h"
#include "condition.h"
#include "creature.h"
#include "player.h"
#include "const76.h"

#include <sstream>

extern Game g_game;

Combat::Combat(CombatType_t _type)
{
	params.condition = NULL;
	area = NULL;
	callback = NULL;
	combatType = _type;
}

Combat::~Combat()
{
	delete params.condition;
	delete area;
	delete callback;
}

void Combat::getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const
{
	if(!creature){
		return;
	}

	if(Player* player = creature->getPlayer()){
		if(callback){
			callback->getMinMaxValues(player, min, max);
		}
		else{
			switch(formula.type){
				case COMBAT_FORMULA_LEVELMAGIC:
					max = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * formula.mina + formula.minb);
					min = (int32_t)((player->getLevel() * 2 + player->getMagicLevel() * 3) * 1. * formula.maxa + formula.maxb);
					break;

				default:
					min = 0;
					max = 0;
					break;
			}

			//std::cout << "No callback set for combat" << std::endl;
		}
	}
	else{
		creature->getCombatValues(min, max);
	}
}

void Combat::getCombatArea(const Position& centerPos, const Position& targetPos, const AreaCombat* area, std::list<Tile*>& list)
{
	if(area){
		area->getList(centerPos, targetPos, list);
	}
	else{
		Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);
		if(tile){
			list.push_back(tile);
		}
	}
}

ReturnValue Combat::canDoCombat(const Creature* caster, const Tile* tile, bool isAggressive)
{
	if(tile->hasProperty(BLOCKPROJECTILE) || tile->hasProperty(BLOCKINGANDNOTMOVEABLE)){
		return RET_NOTENOUGHROOM;
	}

	if(!tile->ground){
		return RET_NOTPOSSIBLE;
	}

	if(tile->floorChange()){
		return RET_NOTENOUGHROOM;
	}

	if(tile->getTeleportItem()){
		return RET_NOTENOUGHROOM;
	}

	if(caster->getPosition().z < tile->getPosition().z){
		return RET_FIRSTGODOWNSTAIRS;
	}

	if(caster->getPosition().z > tile->getPosition().z){
		return RET_FIRSTGOUPSTAIRS;
	}

	if(const Player* player = caster->getPlayer()){
		if(player->getAccessLevel() > 0){
			return RET_NOERROR;
		}
	}

	if(isAggressive && tile->isPz()){
		return RET_ACTIONNOTPERMITTEDINPROTECTIONZONE;
	}

	return RET_NOERROR;
}

void Combat::setArea(const AreaCombat* _area)
{
	area = new AreaCombat(*_area);
}

void Combat::setCondition(const Condition* _condition)
{
	params.condition = _condition->clone();
}

void Combat::setPlayerCombatValues(CombatFormulaType type, double mina, double minb, double maxa, double maxb)
{
	formula.type = type;
	formula.mina = mina;
	formula.minb = minb;
	formula.maxa = maxa;
	formula.maxb = maxb;
}

bool Combat::setParam(CombatParam_t param, uint32_t value)
{
	switch(param){
		case COMBATPARAM_DAMAGETYPE:
		{
			params.damageType = (DamageType_t)value;
			return true;
			break;
		}

		case COMBATPARAM_EFFECT:
		{
			params.impactEffect = value;
			return true;
			break;
		}

		case COMBATPARAM_DISTANCEEFFECT:
		{
			params.distanceEffect = value;
			return true;
			break;
		}

		case COMBATPARAM_BLOCKEDBYARMOR:
		{
			params.blockedByArmor = (value != 0);
			return true;
			break;
		}

		case COMBATPARAM_BLOCKEDBYSHIELD:
		{
			params.blockedByShield = (value != 0);
			return true;
			break;
		}

		case COMBATPARAM_TARGETCASTERORTOPMOST:
		{
			params.targetCasterOrTopMost = (value != 0);
			return true;
			break;
		}

		case COMBATPARAM_CREATEITEM:
		{
			params.itemId = value;
			return true;
			break;
		}

		case COMBATPARAM_AGGRESSIVE:
		{
			params.isAggressive = (value != 0);
		}

		default:
		{
			break;
		}
	}

	return false;
}

bool Combat::setCallback(CombatParam_t key)
{
	switch(key){
		case COMBATPARAM_LEVELMAGICVALUECALLBACK:
			callback = new CombatCallBack(COMBAT_FORMULA_LEVELMAGIC);
			return true;
			break;

		case COMBATPARAM_SKILLVALUECALLBACK:
			callback = new CombatCallBack(COMBAT_FORMULA_SKILL);
			return true;
			break;

		default:
				std::cout << "Combat::setCallback - Unknown callback type: " << (uint32_t)key << std::endl;
				break;
	}

	return false;
}

CallBack* Combat::getCallback()
{
	return callback;
}

bool Combat::CombatHealthFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	Combat2Var* var = (Combat2Var*)data;
	int32_t healthChange = random_range(var->minChange, var->maxChange);
	bool result = g_game.combatChangeHealth(params.damageType, caster, target, healthChange, params.blockedByShield, params.blockedByArmor);

	if(result){
		CombatConditionFunc(caster, target, params, NULL);
	}

	return result;
}

bool Combat::CombatManaFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	Combat2Var* var = (Combat2Var*)data;
	int32_t manaChange = random_range(var->minChange, var->maxChange);
	bool result = g_game.combatChangeMana(caster, target, manaChange);

	if(result){
		CombatConditionFunc(caster, target, params, NULL);
	}

	return result;
}

bool Combat::CombatConditionFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	if(!params.isAggressive || caster != target){
		if(params.condition){
			Condition* conditionCopy = params.condition->clone();
			if(caster){
				conditionCopy->setParam(CONDITIONPARAM_OWNER, caster->getID());
			}

			return target->addCondition(conditionCopy);
		}
	}

	return false;
}

bool Combat::CombatNullFunc(Creature* caster, Creature* target, const CombatParams& params, void* data)
{
	return true;
}

void Combat::combatTileEffects(Creature* caster, Tile* tile, const CombatParams& params)
{
	if(params.itemId != 0){
		Item* item = Item::CreateItem(params.itemId);

		if(caster){
			item->setOwner(caster->getID());
		}

		ReturnValue ret = g_game.internalAddItem(tile, item);
		if(ret != RET_NOERROR){
			delete item;
		}
	}

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(tile->getPosition(), params.impactEffect);
	}
}

void Combat::postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params)
{
	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), pos, params.distanceEffect);
	}
}

void Combat::CombatFunc(Creature* caster, const Position& pos,
	const AreaCombat* area, const CombatParams& params, COMBATFUNC func, void* data)
{
	std::list<Tile*> list;
	getCombatArea(caster->getPosition(), pos, area, list);

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		bool bContinue = true;
		
		if(canDoCombat(caster, *it, params.isAggressive) == RET_NOERROR){
			for(CreatureVector::iterator cit = (*it)->creatures.begin(); bContinue && cit != (*it)->creatures.end(); ++cit){

				if(params.targetCasterOrTopMost){
					if(caster && caster->getTile() == (*it)){
						if(*cit == caster){
							bContinue = false;
						}
					}
					else if(*cit == (*it)->getTopCreature()){
						bContinue = false;
					}
					else{
						continue;
					}
				}
				
				func(caster, *cit, params, data);
			}

			combatTileEffects(caster, *it, params);
		}
	}

	postCombatEffects(caster, pos, params);
}

void Combat::doCombat(Creature* caster, Creature* target) const
{
	//target combat callback function

	if(combatType == COMBAT_HITPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatHealth(caster, target, minChange, maxChange, params);
	}
	else if(combatType == COMBAT_MANAPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatMana(caster, target, minChange, maxChange, params);
	}
	else if(combatType == COMBAT_CONDITION){
		doCombatCondition(caster, target, params);
	}
}

void Combat::doCombat(Creature* caster, const Position& pos) const
{
	//area combat callback function

	if(combatType == COMBAT_HITPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatHealth(caster, pos, area, minChange, maxChange, params);
	}
	else if(combatType == COMBAT_MANAPOINTS){
		int32_t minChange = 0;
		int32_t maxChange = 0;
		getMinMaxValues(caster, minChange, maxChange);

		doCombatMana(caster, pos, area, minChange, maxChange, params);
	}
	else if(combatType == COMBAT_CONDITION){
		doCombatCondition(caster, pos, area, params);
	}
	else{
		CombatFunc(caster, pos, area, params, CombatNullFunc, NULL);
	}
}

void Combat::doCombatHealth(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;
	CombatHealthFunc(caster, target, params, (void*)&var);

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void Combat::doCombatHealth(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;

	CombatFunc(caster, pos, area, params, CombatHealthFunc, (void*)&var);
}

void Combat::doCombatMana(Creature* caster, Creature* target,
	int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;
	CombatManaFunc(caster, target, params, (void*)&var);

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void Combat::doCombatMana(Creature* caster, const Position& pos,
	const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params)
{
	Combat2Var var;
	var.minChange = minChange;
	var.maxChange = maxChange;

	CombatFunc(caster, pos, area, params, CombatManaFunc, (void*)&var);
}

void Combat::doCombatCondition(Creature* caster, const Position& pos, const AreaCombat* area,
	const CombatParams& params)
{
	CombatFunc(caster, pos, area, params, CombatConditionFunc, NULL);
}

void Combat::doCombatCondition(Creature* caster, Creature* target, const CombatParams& params)
{
	CombatConditionFunc(caster, target, params, NULL);	

	if(params.impactEffect != NM_ME_NONE){
		g_game.addMagicEffect(target->getPosition(), params.impactEffect);
	}

	if(caster && params.distanceEffect != NM_ME_NONE){
		g_game.addDistanceEffect(caster->getPosition(), target->getPosition(), params.distanceEffect);
	}
}

void Combat::postCombatEffects(Creature* caster, const Position& pos, bool success) const
{
	Combat::postCombatEffects(caster, pos, params);

	if(!success){
		g_game.addMagicEffect(pos, NM_ME_PUFF);
	}
}


CombatCallBack::CombatCallBack(CombatFormulaType _type)
{
	type = _type;
}

void CombatCallBack::getMinMaxValues(Player* player, int32_t& min, int32_t& max) const
{
	//"onGetPlayerMinMaxValues"(...)
	
	ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
	lua_State* L = m_scriptInterface->getLuaState();
	
	if(!env->setCallbackId(m_scriptId))
		return;
		
	uint32_t cid = env->addThing(player);

	m_scriptInterface->pushFunction(m_scriptId);
	lua_pushnumber(L, cid);

	int32_t parameters = 1;

	switch(type){
		case COMBAT_FORMULA_LEVELMAGIC:
			//"onGetPlayerMinMaxValues"(cid, level, maglevel)
			lua_pushnumber(L, player->getLevel());
			lua_pushnumber(L, player->getMagicLevel());
			parameters += 2;
			break;

		/*
		case COMBAT_FORMULA_SKILL:
			lua_pushnumber(L, player->getSkill(x, SKILL_LEVEL));
			lua_pushnumber(L, (int32_t)minb);
			parameters += 2;
			break;
		*/

		default:
			std::cout << "CombatCallBack::getMinMaxValues - unknown callback type" << std::endl;
			return;
			break;
	}

	int size0 = lua_gettop(L);
	if(lua_pcall(L, parameters, 2 /*nReturnValues*/, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(L)));
	}
	else{
		max = LuaScriptInterface::popNumber(L);
		min = LuaScriptInterface::popNumber(L);
	}

	if((lua_gettop(L) + 3 /*nParams*/  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
}

AreaCombat::AreaCombat()
{
	hasExtArea = false;
}

AreaCombat::~AreaCombat() 
{
	clear();
}

void AreaCombat::clear()
{
	for(AreaCombatMap::iterator it = areas.begin(); it != areas.end(); ++it){
		delete it->second;
	}

	areas.clear();
}

bool AreaCombat::getList(const Position& centerPos, const Position& targetPos, std::list<Tile*>& list) const
{
	Tile* tile = g_game.getTile(targetPos.x, targetPos.y, targetPos.z);

	if(!tile){
		return false;
	}

	const MatrixArea* area = getArea(centerPos, targetPos);
	if(!area){
		return false;
	}

	Position tmpPos = targetPos;

	size_t cols = area->getCols();
	size_t rows = area->getRows();

	uint32_t centerY, centerX;
	area->getCenter(centerY, centerX);

	tmpPos.x -= centerX;
	tmpPos.y -= centerY;

	for(size_t y = 0; y < rows; ++y){
		for(size_t x = 0; x < cols; ++x){
			
			if(area->getValue(y, x) != 0){
				if(g_game.map->canThrowObjectTo(targetPos, tmpPos)){
					tile = g_game.getTile(tmpPos.x, tmpPos.y, tmpPos.z);

					if(tile){
						list.push_back(tile);
					}
				}
			}

			tmpPos.x += 1;
		}

		tmpPos.x -= cols;
		tmpPos.y += 1;
	}

	return true;
}

long round(float v)
{
	long t = (long)std::floor(v);
	if((v - t) > 0.5){
		return t + 1;
	}
	else{
		return t;
	}
}

void AreaCombat::copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const
{
	uint32_t centerY, centerX;
	input->getCenter(centerY, centerX);

	if(op == MATRIXOPERATION_COPY){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			for(unsigned int x = 0; x < input->getCols(); ++x){
				(*output)[y][x] = (*input)[y][x];
			}
		}

		output->setCenter(centerY, centerX);
	}
	else if(op == MATRIXOPERATION_MIRROR){
		for(unsigned int y = 0; y < input->getRows(); ++y){
			int rx = 0;
			for(int x = input->getCols() - 1; x >= 0; --x){
				(*output)[y][rx++] = (*input)[y][x];
			}
		}
		
		output->setCenter(centerY, (input->getRows() - 1) - centerX);
	}
	else if(op == MATRIXOPERATION_FLIP){
		for(unsigned int x = 0; x < input->getCols(); ++x){
			int ry = 0;
			for(int y = input->getRows() - 1; y >= 0; --y){
				(*output)[ry++][x] = (*input)[y][x];
			}
		}

		output->setCenter((input->getCols() - 1) - centerY, centerX);
	}
	//rotation
	else{
		uint32_t centerX, centerY;
		input->getCenter(centerY, centerX);

		int32_t rotateCenterX = (output->getCols() / 2) - 1;
		int32_t rotateCenterY = (output->getRows() / 2) - 1;
		int32_t angle = 0;

		switch(op){
			case MATRIXOPERATION_ROTATE90:
				angle = 90;
				break;

			case MATRIXOPERATION_ROTATE180:
				angle = 180;
				break;

			case MATRIXOPERATION_ROTATE270:
				angle = 270;
				break;

			default:
				angle = 0;
				break;
		}
		double angleRad = 3.1416 * angle / 180.0;

		float a = std::cos(angleRad);
		float b = -std::sin(angleRad);
		float c = std::sin(angleRad);
		float d = std::cos(angleRad);
		
		for(unsigned int x = 0; x < input->getCols(); ++x){
			for(unsigned int y = 0; y < input->getRows(); ++y){
				//calculate new coordinates using rotation center
				long newX = x - centerX;
				long newY = y - centerY;

				//perform rotation
				long rotatedX = round(newX * a + newY * b);
				long rotatedY = round(newX * c + newY * d);

				//write in the output matrix using rotated coordinates
				(*output)[rotatedY + rotateCenterY][rotatedX + rotateCenterX] = (*input)[y][x];
			}
		}

		output->setCenter(rotateCenterY, rotateCenterX);
	}
}

MatrixArea* AreaCombat::createArea(const std::list<uint32_t>& list, uint32_t rows)
{
	unsigned int cols = list.size() / rows;
	MatrixArea* area = new MatrixArea(rows, cols);

	uint32_t x = 0;
	uint32_t y = 0;

	for(std::list<uint32_t>::const_iterator it = list.begin(); it != list.end(); ++it){
		if(*it == 1 || *it == 3){
			area->setValue(y, x, true);
		}
		
		if(*it == 2 || *it == 3){
			area->setCenter(y, x);
		}

		++x;

		if(cols == x){
			x = 0;
			++y;
		}
	}

	return area;
}

void AreaCombat::setupArea(const std::list<uint32_t>& list, uint32_t rows)
{
	MatrixArea* area = createArea(list, rows);

	//NORTH
	areas[NORTH] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//SOUTH
	MatrixArea* southArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, southArea, MATRIXOPERATION_ROTATE180);
	areas[SOUTH] = southArea;

	//EAST
	MatrixArea* eastArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, eastArea, MATRIXOPERATION_ROTATE90);
	areas[EAST] = eastArea;

	//WEST
	MatrixArea* westArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, westArea, MATRIXOPERATION_ROTATE270);
	areas[WEST] = westArea;
}

void AreaCombat::setupExtArea(const std::list<uint32_t>& list, uint32_t rows)
{
	if(list.empty()){
		return;
	}

	hasExtArea = true;
	MatrixArea* area = createArea(list, rows);

	//NORTH-WEST
	areas[NORTHWEST] = area;

	uint32_t maxOutput = std::max(area->getCols(), area->getRows()) * 2;

	//NORTH-EAST
	MatrixArea* neArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, neArea, MATRIXOPERATION_MIRROR);
	areas[NORTHEAST] = neArea;

	//SOUTH-WEST
	MatrixArea* swArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(area, swArea, MATRIXOPERATION_FLIP);
	areas[SOUTHWEST] = swArea;

	//SOUTH-EAST
	MatrixArea* seArea = new MatrixArea(maxOutput, maxOutput);
	copyArea(swArea, seArea, MATRIXOPERATION_MIRROR);
	areas[SOUTHEAST] = seArea;
}

MagicField::MagicField(uint16_t _type) : Item(_type)
{
	condition = NULL;
	damageType = DAMAGE_NONE;

	load();
}

void MagicField::load()
{
	const ItemType& it = Item::items[getID()];

	damageType = it.damageType;

	switch(damageType){
		case DAMAGE_ENERGY:
			condition = new ConditionDamage(CONDITION_ENERGY);
			break;

		case DAMAGE_FIRE:
			condition = new ConditionDamage(CONDITION_FIRE);
			break;

		case DAMAGE_POISON:
			condition = new ConditionDamage(CONDITION_POISON);
			break;
		
		default:
			break;
	}

	if(condition){
		if(it.initialDamage != 0){
			condition->addDamage(1, 0, it.initialDamage);
		}

		condition->addDamage(it.roundMin, it.roundTime, it.roundDamage);
	}
}

MagicField::~MagicField()
{
	//
}

DamageType_t MagicField::getDamageType() const
{
	return damageType;
}

void MagicField::onStepInField(Creature* creature)
{
	//remove magic walls/wild growth
	if(isBlocking()){
		g_game.internalRemoveItem(this, 1);
	}
	else{
		if(condition){
			Condition* conditionCopy = condition->clone();
			uint32_t owner = getOwner();
			if(owner != 0){
				conditionCopy->setParam(CONDITIONPARAM_OWNER, owner);
			}

			creature->addCondition(conditionCopy);
		}
	}
}
