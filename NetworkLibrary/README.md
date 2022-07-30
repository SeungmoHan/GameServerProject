# Lan Server, NetServer

- 가장 기본이 되는 구조의 설계모양

  1. Lan Server : 인코딩 부재 WORD type의 페이로드 길이만 포함 

  2. 서버와 서버사이의 통신이기 때문에 사설 네트워크 환경에서만 작동하므로 패킷에 대한 추가적인 확인과정 필요 없음.

     ​	LanPacketHeader

     ​	{

     ​		WORD _PayloadSize;

     ​	}

  3. Net Server : 인코딩 존재 패킷에 담아 보내는 랜덤키(패킷 보내기 직전에 rand()를 통해서 얻음)와 서버와 클라가 공유하는 하나의 고정키 존재(고정키는 config 파일을 통해서 read)  외부망에 존재하는 클라이언트와의 통신이기 때문에 올바른 패킷인지 아닌지에 대한 여부를 네트워크 라이브러리 차원에서 검증

     ​	NetPacketHeader

     ​	{

     ​		BYTE _PacketCode;

     ​		WORD _PayloadSize;

     ​		BYTE _RandKey;

     ​		BYTE _CheckSum;

     ​	}

- 한계점
  1. 로직을 돌리는 쓰레드가 여러개가 나올 경우 어느 쓰레드로 JobEnqueue를 해야 할지에 대한 여부를 라이브러리 차원에서 지원이 불가능함(컨텐츠가 이를 관리해야됨)



# 개선된 방식의 Thread Block Attach 방식의 GameServer

- GameServer 객체를 바로 이용 + BaseServer의 BasicThreadBlock클래스를 상속받은 클래스(로직부)만 구현

- 기능
  1. BasicThreadBlock클래스를 상속받아 구현해서 GameServer객체에 Attach 해주면 네트워크 라이브러리에서 핸들링
  
  2. 라이브러리 차원에서 로직을 돌리는 쓰레드블럭들을 관리하기 때문에 이전의 NetServer구조보다 컨텐츠 구현시 고민거리가 일부 줄어듦
  
  3. Attach된 쓰레드 블럭에 대한 접근은 쓰레드 블럭의 이름으로 접근 가능하며 이 이름은 컨텐츠에서(쓰레드 블럭 혹은 그외에서) 보관해야됨
  
  4. 플레이어가 다른 쓰레드 블럭으로 이동가능(일종의 채널(지역)간 이동 기능)
  
  5. 유저의 쓰레드 블럭간 이동은 메시지를 통해서 전달
  
     5-1. 동시에 모든 쓰레드 블럭에 존재하지 않는 경우 발생 + 이상황에 메시지가 처리되어야 하는 상황이 발생하면 문제가 생김
  
     5-2. 메시지를 받은 쓰레드 블럭에 없으면 GameServer에 있는지 확인후 없으면 Disconnect, 있으면 유저가 존재하는 쓰레드 블럭에 다시 큐잉하는 방식으로 해결
