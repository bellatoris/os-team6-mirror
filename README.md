# os-team6
Project1
--------------------------
1.How to build kernel
2.High-level design and implemetation
3.Investigation of the process tree

  ii
    타이젠에서 앱을 실행시키면 launchpad의 child로 실행시킨 앱의 프로세스 하나가 추가된다 
    앱을 종료하면 해당하는 프로세스가 종료된다.
  iii.a
	  타이젠 상에서 앱을 실행시키면 launchpad-process의 child인 launchpad-loader중 에서
	  하나가 그 앱의 이름으로 바뀐다.이때 이름을 제외한 pid, uid 등의 field값들이
	  변하지 않는 것으로 보아 새로운 앱이 생성되는 것이 아니라 loader중 하나가 바뀌는 것을 알 수 있다.
	  그와 동시에 새로운 launchpad-loader가 생성되어 총 3개의 Loader가 유지 된다.
	iii.b
	  launchpad는 앱을 실행 시키는 명령을 받으면 권한이 있는지 판단해서 앱을 띄우는 역할을 한다.
	  또한 Loader,앱의 fork,exit등을 관리한다. 
	  Loader는 앱을 구성하는데필수적인 요소들을 미리 만들어서 앱의 실행 속도를 빠르게 한다. 
