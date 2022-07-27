# Chatting Server



- select 모델로 만들어진 서버
- Spec
  1. Windows환경에서 개발
  2. c++ 구현
  3. VS 2019
- 기능
  1. 로그인
  2. 방 생성
  3. 방 입장
  4. 채팅
  5. 방 나가기
  6. select 기반 동기식 IO 사용



# TCP Fighter

- 클라 : Async Select 모델 사용
- 서버 : Select 모델 사용
- Spec
  1. Windows 환경에서 개발
  2. 클라이언트는 윈도우 종속적(Windows API사용 및 Async Select 사용)
  3. c++ 구현
- 서버
  1. 플레이어 기준 주변 9개의 섹터에만 데이터 전달
  2. 좌표가 틀어지면 sync packet 전달
  3. 충돌판정
- 클라
  1. 드래그, 창 내림 등으로 처리되지 못한 로직 처리를 위해 렌더 스킵 구현
