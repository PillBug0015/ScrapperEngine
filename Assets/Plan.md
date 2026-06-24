# GameDatabase 
- 원본 데이터 서고
## Data categories(!readonly!)
- EntityData
- EventScript
- BattleActionData
- ItemData
- ShopData
- ModifierData


# RuntimeState / GameRuntime
- 현재 저장 데이터
## expected field
- party(vector<entity>)
- inventory
- current scene id
- flags
- variables

# GameManager
- 부팅/연결자 및 사회자 (Update 주입기)
- GameDatabase에서 대본을 꺼냄
- RuntimeState를 준비함
- EventSystem에게 Script + RuntimeState를 넘김

# Systems
## EventSystem
- Story VM / 낭독자
- 받은 대본을 순서대로 실행
- Text/Choice/Battle/Shop/Reward 명령을 해석
- Battle/Shop 등으로 mode 전환 요청

## BattleSystem
- Battle VM
- BattleActionData를 해석
- RuntimeState와 BattleState를 변경

### ShopSystem 등등이 들어올 수도...