void soil_setup() {
  ADMUX |= (1 << REFS0);
  ADCSRA |= (1 << ADEN) | (0 << ADSC) | (0 << ADATE);
}

uint16_t ADC_read() {
  ADCSRA |= (1 << ADSC);
   while (ADCSRA & (1 << ADSC));
  ADC = (ADCL | (ADCH << 8));
  return (ADC);
}

uint16_t get_soil_moisture() {
    return ADC_read();
}
