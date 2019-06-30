/*especificaciones del led*/
ltl spec1{
[]((estado_led==APAGADO && contraseña==1 && presencia==1)->(<>estado_led==ENCENDIDO))
}
ltl spec2{
[]((estado_led==ENCENDIDO)->(<>(estado_led==APAGADO)))
}
/*especificaciones de la alarma*/
ltl spec3{
[](((estado_alarma==IDLE)&&(contraseña==1))->(<>(estado_alarma==ARMADO)))
}
ltl spec4{
[]((estado_alarma==CIFRA0 && time>timeout)->(<>(estado_alarma==CIFRA1)))
}
ltl spec5{
[]((estado_alarma==CIFRA1 && time>timeout)->(<>(estado_alarma==CIFRA2)))
}
ltl spec6{
[]((estado_alarma==CIFRA2 && time>timeout)->(<>(estado_alarma==IDLE)))
}

/*MAQUINAS DE ESTADO*/
#define segundo 1000
mtype={APAGADO,ENCENDIDO,IDLE,CIFRA0,CIFRA1,CIFRA2}
int estado_led;
int estado_alarma;
int presencia;
int contraseña;
int boton;
int timeout_1s;
int time;
int cuenta;
int contraseña_0=1;
int contraseña_1=2;
int contraseña_2=3;
int aux_0;
int aux_1;

/*PROCTYPE*/
active proctype led(){
	estado_led=APAGADO;
	do
	::(estado_led==APAGADO)-> atomic{
		if
		::((presencia==1)&& contraseña==1)->boton=0;estado_led=ENCENDIDO;timeout_1s=time+segundo;
		fi
	}
	::(estado_led==ENCENDIDO)->atomic{
		if
		::(time>timeout_1s)->estado_led=APAGADO;
		fi
	}
	od
}
active proctype alarma(){
	estado_alarma=IDLE
	do
	::(estado_alarma==IDLE)-> atomic{
		if 
		::(boton==1)->boton=0;cuenta=0;contraseña=0;timeout_1s=time+segundo;aux_0=0;aux_1=0;
		fi
	}
	::(estado_alarma==CIFRA0)-> atomic{
		if
		::(time>timeout_1s)->estado_alarma=CIFRA1;cuenta=0;timeout_1s=time+segundo;
		::(boton==1)->boton=0;cuenta=cuenta+1;timeout_1s=time+segundo;
		::(cuenta==contraseña_0)->aux_0=1;
		::(cuenta!=contraseña_0)->aux_0=0;
		fi
	}
	::(estado_alarma==CIFRA1)-> atomic{
		if
		::(time>timeout_1s)->estado_alarma=CIFRA2;cuenta=0;timeout_1s=time+segundo;
		::(boton==1)->boton=0;cuenta=cuenta+1;timeout_1s=time+segundo;
		::(cuenta==contraseña_1 && aux_0)->aux_1=1;
		::(cuenta!=contraseña_1)->aux_1=0;
		fi
	}
	::(estado_alarma==CIFRA2)-> atomic{
		if
		::(time>timeout_1s)->estado_alarma=IDLE;cuenta=0;timeout_1s=time+segundo;
		::(boton==1)->boton=0;cuenta=cuenta+1;timeout_1s=time+segundo;
		::(cuenta==contraseña_2 && aux_0 && aux_1)->contraseña=1;
		::(cuenta!=contraseña_2)->contraseña=0;
		fi
	}
	od
}
active proctype entorno_1(){
	time=0;
	do
	::if
		::boton=1
		::presencia=1
		::skip
	  fi
	  time=time+1;
	  printf("tiempo=%d,estado alarma=%e, estado led=%e,contraseña correcta=%d",time,estado_alarma,estado_led,contraseña);
	od
}
