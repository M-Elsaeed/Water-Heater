//Measuring temperature
/*
  lcd_cmd(L_CLR);
  lcd_cmd(L_L1);
  lcd_str("   Teste TEMP");
  
  TRISA=0x07;

  adc_init();

  for(i=0; i< 100; i++)
  {
    tmpi=(adc_amostra(2)*10)/2;
    lcd_cmd(L_L2+5);
    itoa(tmpi,str);
    lcd_dat(str[2]);
    lcd_dat(str[3]);
    lcd_dat(',');
    lcd_dat(str[4]);
    lcd_dat('C');
    atraso_ms(20);
  }
*/



//Heating
/*
  lcd_cmd(L_CLR);
  lcd_cmd(L_L1);
  lcd_str("   Teste AQUEC");
  PORTCbits.RC5=1;
  for(i=0; i< 100; i++)
  {
    tmpi=(adc_amostra(2)*10)/2;
    lcd_cmd(L_L2+5);
    itoa(tmpi,str);
    lcd_dat(str[2]);
    lcd_dat(str[3]);
    lcd_dat(',');
    lcd_dat(str[4]);
    lcd_dat('C');
    atraso_ms(50);
  }
  PORTCbits.RC5=0;

*/

// Cooling

/*
  TRISCbits.TRISC0=1;
  lcd_cmd(L_CLR);
  lcd_cmd(L_L1);
  lcd_str("   Teste RESFR");

//timer0 temporizador
#if defined(_16F877A) || defined(_16F777)  
 OPTION_REGbits.T0CS=0;
 OPTION_REGbits.PSA=0;
 OPTION_REGbits.PS0=0;
 OPTION_REGbits.PS1=0;
 OPTION_REGbits.PS2=1;
#else
 T0CONbits.T0CS=0;
 T0CONbits.PSA=0;
 T0CONbits.T08BIT=1;
 T0CONbits.T0PS0=0; // divide por 32
 T0CONbits.T0PS1=0;
 T0CONbits.T0PS2=1;
 T0CONbits.TMR0ON=1;
#endif
 
 
 INTCONbits.T0IE=1;

//T = 32x250x125 = 1segundo;

//timer1 contador
 T1CONbits.TMR1CS=1;
 T1CONbits.T1CKPS1=0;
 T1CONbits.T1CKPS0=0;


 INTCONbits.T0IF=0;
#if defined(_16F877A) || defined(_16F777)  
 TMR0=6;
#else
 TMR0H=0;
 TMR0L=6; //250
#endif
 cnt=125; 
 INTCONbits.GIE=1;

 TMR1H=0;
 TMR1L=0;
 T1CONbits.TMR1ON=1;

  PORTCbits.RC2=1;
  for(i=0; i< 150; i++)
  {
    tmpi=(adc_amostra(2)*10)/2;
    lcd_cmd(L_L2+2);
    itoa(tmpi,str);
    lcd_dat(str[2]);
    lcd_dat(str[3]);
    lcd_dat(',');
    lcd_dat(str[4]);
    lcd_dat('C');

    lcd_cmd(L_L2+8);
    itoa(t1cont,str);
    lcd_dat(str[1]);
    lcd_dat(str[2]);
    lcd_dat(str[3]);
    lcd_dat(str[4]);
    lcd_dat('R');
    lcd_dat('P');
    lcd_dat('S');

    atraso_ms(10);
  }

  INTCONbits.GIE=0;
  PORTCbits.RC2=0;


#ifdef _18F452
  ADCON1=0x06;
#else
  ADCON1=0x0F;
#endif
*/


//teste EEPROM EXT
/*
  lcd_cmd(L_CLR);
  lcd_cmd(L_L1);
  lcd_str("Teste EEPROM EXT");
// testar ? 
  lcd_cmd(L_L2);
  lcd_str(" (s=RB0 n=RB1) ?");

  TRISB=0x03;

  while(PORTBbits.RB0 && PORTBbits.RB1);

  if(PORTBbits.RB0 == 0)
  {
    tmp=e2pext_r(10);
    lcd_dat(tmp);

    e2pext_w(10,0xA5);
    e2pext_w(10,0x5A);
    i=e2pext_r(10);

    e2pext_w(10,tmp);

    lcd_cmd(L_CLR);
    lcd_cmd(L_L1);
    lcd_str("Teste EEPROM EXT");
    lcd_cmd(L_L2);
    if(i == 0x5A) 
      lcd_str("       OK");
    else
      lcd_str("      ERRO");

    atraso_ms(1000);
  }
  else
  {
    while(PORTBbits.RB1 == 0);
  }
#endif
  TRISB=0x00;
  PORTB=0;
*/