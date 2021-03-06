#include "src/utils/Arduboy2Ext.h"
#include <ArduboyTones.h>
#include "src/images/sounds.h"

void clearEnemyMovementPositions(Enemy *enemies) {
  
  for (uint8_t x = 0; x < NUMBER_OF_ENEMIES; x++) {

    Enemy *enemy = &enemies[x];
    enemy->setXFuturePosition(0);
    enemy->setYFuturePosition(0);

  }
  
}

void enemyMovements(Enemy *enemy) {

  uint8_t enemyX = enemy->getX() / GRID_SIZE;
  uint8_t enemyY = enemy->getY() / GRID_SIZE;


  // Check to see if the enemy has touched gold!

  if (level.getLevelData(enemyX, enemyY) == LevelElement::Gold && enemy->getHasGold() == 0) {

    if (random(0, ENEMY_GOLD_PICKUP_THRESHOLD) == 0) {

      enemy->setHasGold(random(ENEMY_GOLD_HOLD_MINIMUM, ENEMY_GOLD_HOLD_MAXIMUM));
      level.setLevelData(enemyX, enemyY, LevelElement::Blank);

    }

  }


  // Move enemy ..

  switch (enemy->getStance()) {


    // If the enemy is being reborn, return without moving ..

    case PlayerStance::Rebirth_1 ... PlayerStance::Rebirth_3:
      break;


    // If the enemy is falling, then continue falling ..

    case PlayerStance::Falling:
      {
        LevelElement current = level.getLevelData(enemyX, enemyY);
        LevelElement down = level.getLevelData(enemyX, enemyY + 1);

        if (current != LevelElement::Brick_Transition && canContinueToFall_Enemy(down)) {
          break;
        }

      }


    // Otherwise move the enemy towards the player ..

    default:

      if (enemy->getX() % GRID_SIZE == 0 && enemy->getY() % GRID_SIZE == 0) {


        bool hasMoved = false;
        enemyX = enemy->getX() / GRID_SIZE;
        enemyY = enemy->getY() / GRID_SIZE;

        LevelElement current =    level.getLevelData(enemyX, enemyY);


        // If the enemy is in a hole, then attemt to wiggle out ..

        if (enemy->getEscapeHole() > EscapeHole::None) {


          // If the enemy has gold, then make it available to pickup ..

          if (enemy->getHasGold() > ENEMY_GOLD_DROP_VALUE) {

            enemy->setHasGold(0);
            level.setLevelData(enemyX, enemyY - 1, LevelElement::Gold);

          }



          // Check to see if the enemy can continue falling ..
            
          enemy->setYDelta(0);
          enemy->setEscapeHole(static_cast<EscapeHole>(static_cast<uint8_t>(enemy->getEscapeHole()) - 1));

        }


        // Otherwise move the enemy towards the player ..

        else {

          Direction direction = Direction::Up;

          LevelElement up =         level.getLevelData(enemyX, enemyY - 1);
          LevelElement right =      level.getLevelData(enemyX + 1, enemyY);
          LevelElement rightDown =  level.getLevelData(enemyX + 1, enemyY + 1);
          LevelElement leftDown =   level.getLevelData(enemyX - 1, enemyY + 1);
          LevelElement left =       level.getLevelData(enemyX - 1, enemyY);
          LevelElement down =       level.getLevelData(enemyX, enemyY + 1);

          int16_t xDiff = enemy->getX() - (player.getX() - level.getXOffset());
          int16_t yDiff = enemy->getY() - (player.getY() - level.getYOffset());


          if (enemy->getDirectionCount() > 0) {


            // Can the enemy move in the preferred direction ?

            hasMoved = attemptToMove(enemy, enemyX, enemyY, enemy->getPreferredDirection(), current, up, right, rightDown, down, leftDown, left, false);
            enemy->setDirectionCount(enemy->getDirectionCount() - 1);

            if (hasMoved) {
              return;
            }
            else {  
              direction = enemy->getDirection();
            }

          }
          else {

            direction = getDirection_16Directions(xDiff, yDiff);
            enemy->setPreferredDirection(getDirection_4Directions(direction));

          }

          Direction direction1 = direction;
          Direction direction2 = direction;



          // Drop the gold?

          if (enemy->getHasGold() > 0 && isSolid(down) && current == LevelElement::Blank) {

            enemy->setHasGold(enemy->getHasGold() - 1);

            if (enemy->getHasGold() == ENEMY_GOLD_DROP_VALUE) {

              level.setLevelData(enemyX, enemyY, LevelElement::Gold);

            }

          }
 


          // Can the enemy move in the first direction ?

          hasMoved = attemptToMove(enemy, enemyX, enemyY, direction, current, up, right, rightDown, down, leftDown, left, true);


          // If not, try to move in the closest possible direction to the preferred ..

          if (!hasMoved) {

            for (uint8_t x = 0; x < 8; x++) {

              if (!hasMoved) { 
                
                direction1--; 
                hasMoved = attemptToMove(enemy, enemyX, enemyY, direction1, current, up, right, rightDown, down, leftDown, left, true); 
                if (hasMoved) break;
                
              }
          
              if (!hasMoved) { 
                
                direction2++; 
                hasMoved = attemptToMove(enemy, enemyX, enemyY, direction2, current, up, right, rightDown, down, leftDown, left, true); 
                if (hasMoved) break;

              }

            }

          }


          // If no movement and we are falling, check to see if we have hit the bottom ..

          if (!hasMoved && enemy->getStance() == PlayerStance::Falling) {

            if (canBeStoodOn_Enemy(down)) {

               enemy->setYDelta(0);

            }

          }

        }  

      }

      break;

  }

}

boolean isOccupiedByAnotherEnemy(Enemy *enemies, Enemy *enemy, int8_t xDelta, int8_t yDelta) {

  for (uint8_t x = 0; x < NUMBER_OF_ENEMIES; x++) {

    Enemy *testEnemy = &enemies[x];

    if (testEnemy->getEnabled() && testEnemy->getId() != enemy->getId()) {

      Rect testRect = { static_cast<int16_t>(testEnemy->getX() + testEnemy->getXFuturePosition()), static_cast<int16_t>(testEnemy->getY() + testEnemy->getYFuturePosition()), GRID_SIZE, GRID_SIZE };
      Rect enemyRect = { static_cast<int16_t>(enemy->getX() + xDelta), static_cast<int16_t>(enemy->getY() + yDelta), GRID_SIZE, GRID_SIZE };

      if (arduboy.collide(testRect, enemyRect)) {

        return true;

      }

    }

  }

  return false;

}

boolean attemptToMove(Enemy *enemy, uint8_t enemyX, uint8_t enemyY, 
                      Direction direction, 
                      LevelElement current, LevelElement up,  
                      LevelElement right, LevelElement rightDown, LevelElement down,
                      LevelElement leftDown, LevelElement left, bool randomMoves) {

  bool hasMoved = false;

  switch (direction) {

    case Direction::LeftUp ... Direction::LeftUp2:
    case Direction::Up     ... Direction::RightUp:
      {
        bool canBeOccupied = canBeOccupiedBasic_Enemy(up);
        bool occupiedByAnotherEnemy = !isOccupiedByAnotherEnemy(enemies, enemy, 0, -GRID_SIZE);

        if ( current == LevelElement::Ladder && canBeOccupied && occupiedByAnotherEnemy) {

          updateEnemyStance(enemy, PlayerStance::Climbing_Up1, PlayerStance::Climbing_Up2);   
          move(enemy, 0, -2, randomMoves);
          hasMoved = true;

        }

      }
      break;

    case Direction::RightUp2 ... Direction::RightDown1:
      {
        bool notOccupiedByAnotherEnemy_Right = !isOccupiedByAnotherEnemy(enemies, enemy, GRID_SIZE, 0);
        bool canBeFallenInto_Down = canBeFallenInto_Enemy(down, enemies, enemyX, enemyY + 1);
        bool canBeFallenInto_RightDown = canBeFallenInto_Enemy(rightDown, enemies, enemyX + 1, enemyY + 1);
        bool canBeOccupied_Right = canBeOccupied_Enemy(right);
        bool canBeStoodOn_RightDown = canBeStoodOn_Enemy(rightDown);

        if (right == LevelElement::Rail && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Swinging_Right1, PlayerStance::Swinging_Right4);
          move(enemy, 2, 0, randomMoves);
          hasMoved = true;

        }

        else if (current == LevelElement::Rail && canBeOccupied_Right && canBeStoodOn_RightDown && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Running_Right1, PlayerStance::Running_Right4);
          move(enemy, 2, 0, randomMoves);
          hasMoved = true;

        }

        else if (current == LevelElement::Ladder && canBeOccupied_Right && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Climbing_Up1, PlayerStance::Climbing_Up2);
          move(enemy, 2, 0, randomMoves);
          hasMoved = true;

        }

        else if (canBeFallenInto_Down) {

          enemy->setStance(PlayerStance::Falling);
          move(enemy, 0, 2, randomMoves);
          hasMoved = true;


          // If we have fallen into a brick hole, start counting ..

          switch (down) {

            case LevelElement::Brick_1 ... LevelElement::Brick_Close_4:
              enemy->setEscapeHole(EscapeHole::WaitMax);
              sound.tones(enemyFallsIn);
              break;
            
            default:  break;

          }

        }

        else if (canBeStoodOn_RightDown && canBeOccupied_Right && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Running_Right1, PlayerStance::Running_Right4);
          move(enemy, 2, 0, randomMoves);
          hasMoved = true;

        }

        else if (canBeOccupied_Right && canBeFallenInto_RightDown && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Running_Right1, PlayerStance::Running_Right4);
          move(enemy, 2, 0, randomMoves);
          hasMoved = true;

        }
    
      }

      break;

    case Direction::RightDown ... Direction::LeftDown:
      {
        bool notOccupiedByAnotherEnemy_Right = !isOccupiedByAnotherEnemy(enemies, enemy, 0, GRID_SIZE);
        bool canBeFallenInto_Down = canBeFallenInto_Enemy(down, enemies, enemyX, enemyY + 1);

        if (down == LevelElement::Ladder && notOccupiedByAnotherEnemy_Right) {

          updateEnemyStance(enemy, PlayerStance::Climbing_Down2, PlayerStance::Climbing_Down1);
          move(enemy, 0, 2, randomMoves);
          hasMoved = true;

        }

        else if (canBeFallenInto_Down && notOccupiedByAnotherEnemy_Right) {

          enemy->setStance(PlayerStance::Falling);
          move(enemy, 0, 2, randomMoves);
          hasMoved = true;


          // If we have fallen into a brick hole, start counting ..

          switch (down) {

            case LevelElement::Brick_1 ... LevelElement::Brick_Close_4:
              enemy->setEscapeHole(EscapeHole::WaitMax);
              break;
            
            default:  break;

          }

        }

      }

      break;

    case Direction::LeftDown2 ... Direction::LeftUp1:
      {

        bool notOccupiedByAnotherEnemy_Left = !isOccupiedByAnotherEnemy(enemies, enemy, -GRID_SIZE, 0);
        bool canBeFallenInto_Down = canBeFallenInto_Enemy(down, enemies, enemyX, enemyY + 1);
        bool canBeFallenInto_LeftDown = canBeFallenInto_Enemy(leftDown, enemies, enemyX - 1, enemyY + 1);
        bool canBeOccupied_Left = canBeOccupied_Enemy(left);
        bool canBeStoodOn_LeftDown = canBeStoodOn_Enemy(leftDown);

        if (left == LevelElement::Rail && notOccupiedByAnotherEnemy_Left) {

          updateEnemyStance(enemy, PlayerStance::Swinging_Left4, PlayerStance::Swinging_Left1);
          move(enemy, -2, 0, randomMoves);
          hasMoved = true;

        }
        
        else if (current == LevelElement::Rail && canBeOccupied_Left && canBeStoodOn_LeftDown && notOccupiedByAnotherEnemy_Left) {

          updateEnemyStance(enemy, PlayerStance::Running_Left4, PlayerStance::Running_Left1);
          move(enemy, -2, 0, randomMoves);
          hasMoved = true;

        }
        
        else if (current == LevelElement::Ladder && canBeOccupied_Left && notOccupiedByAnotherEnemy_Left) {

          updateEnemyStance(enemy, PlayerStance::Climbing_Up1, PlayerStance::Climbing_Up2);
          move(enemy, -2, 0, randomMoves);
          hasMoved = true;

        }

        else if (canBeFallenInto_Down) {

          enemy->setStance(PlayerStance::Falling);
          move(enemy, 0, 2, randomMoves);
          hasMoved = true;


          // If we have fallen into a brick hole, start counting ..

          switch (down) {

            case LevelElement::Brick_1 ... LevelElement::Brick_Close_4:
              enemy->setEscapeHole(EscapeHole::WaitMax);
              sound.tones(enemyFallsIn);
              break;
            
            default:  break;

          }

        }
              
        else if (canBeStoodOn_LeftDown && canBeOccupied_Left && notOccupiedByAnotherEnemy_Left) {

          updateEnemyStance(enemy, PlayerStance::Running_Left4, PlayerStance::Running_Left1);
          move(enemy, -2, 0, randomMoves);
          hasMoved = true;

        }

        else if (canBeOccupied_Left && canBeFallenInto_LeftDown && notOccupiedByAnotherEnemy_Left) {

          updateEnemyStance(enemy, PlayerStance::Swinging_Left4, PlayerStance::Swinging_Left1);
          move(enemy, -2, 0, randomMoves);
          hasMoved = true;

        }

      }

      break;

    default:  break;

  }

  return hasMoved;
  
}


void setDirectionAfterHoleEscape(Enemy *enemy) {

  uint8_t enemyX = enemy->getX() / GRID_SIZE;
  uint8_t enemyY = enemy->getY() / GRID_SIZE;

  LevelElement left       = level.getLevelData(enemyX - 1, enemyY);
  LevelElement leftDown   = level.getLevelData(enemyX - 1, enemyY + 1);
  LevelElement right      = level.getLevelData(enemyX + 1, enemyY);
  LevelElement rightDown  = level.getLevelData(enemyX + 1, enemyY + 1);

  if (static_cast<int16_t>(enemy->getX()) > (static_cast<int16_t>(player.getX()) - level.getXOffset())) {

    if (canBeOccupied_Enemy(left) && canBeStoodOn_Enemy(leftDown)) {
    
      enemy->setStance(PlayerStance::Running_Left1);
      enemy->setXDelta(-2);
      
    }
    else {
    
      enemy->setStance(PlayerStance::Running_Right1);
      enemy->setXDelta(2);

    }

  }
  else {

    if (canBeOccupied_Enemy(right) && canBeStoodOn_Enemy(rightDown)) {

      enemy->setStance(PlayerStance::Running_Right1);
      enemy->setXDelta(2);
      
    }
    else {

      enemy->setStance(PlayerStance::Running_Left1);
      enemy->setXDelta(-2);
      
    }

  }

}

void move(Enemy *enemy, int8_t x, int8_t y, bool randomMoves) {

  enemy->setXDelta(x);
  enemy->setYDelta(y);
  enemy->setXFuturePosition(HALF_GRID_SIZE * x);
  enemy->setYFuturePosition(HALF_GRID_SIZE * y);

  if (x < 0) { enemy->setDirection(Direction::Left); }
  if (x > 0) { enemy->setDirection(Direction::Right); }
  if (y < 0) { enemy->setDirection(Direction::Up); }
  if (y > 0) { enemy->setDirection(Direction::Down); }
  
  if (randomMoves && enemy->getDirectionCount() == 0) { enemy->setDirectionCount(random(0, 6)); }

}


// ------------------------------------------------------------------------------------------
//  Update enemy stance if appropriate ..
// ------------------------------------------------------------------------------------------

void updateEnemyStance(Enemy *enemy, PlayerStance lowerRange, PlayerStance upperRange) {

  if (enemy->getStance() < lowerRange || enemy->getStance() > upperRange) enemy->setStance(lowerRange);
  
}