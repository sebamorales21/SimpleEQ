/*
  ==============================================================================

    LookAndFeel.cpp
    Created: 30 Dec 2025 11:20:02pm
    Author:  usuario

  ==============================================================================
*/

#include "LookAndFeel.h"

KnobLookAndFeel::KnobLookAndFeel()
{
    knobStrip = juce::ImageCache::getFromMemory(BinaryData::Main_Knob_png, BinaryData::Main_Knob_pngSize);

    jassert(!knobStrip.isNull());

    frameSize = knobStrip.getWidth();              // 64 px
    numFrames = knobStrip.getHeight() / frameSize; // 1984/64 = 31 frames
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y,
    int width, int height,
    float sliderPosProportional,
    float /*rotaryStartAngle*/,
    float /*rotaryEndAngle*/,
    juce::Slider& /*slider*/)
{
	// Primero verificamos que la imagen se haya cargado correctamente
    if (knobStrip.isNull())
        return;

	// Aca calculamos el frameIndex basado en la posicion del slider (0.0 a 1.0)
	// frameIndex es un entero entre 0 y numFrames - 1, que indica que frame de la filmstrip se debe dibujar
	// juce::jlimit se asegura que el valor este dentro del rango especificado
    const int frameIndex = juce::jlimit(0, numFrames - 1, (int)std::round(sliderPosProportional * (numFrames - 1)));

	// Luego calculamos las coordenadas de origen y destino para dibujar la imagen, con drawImage
	// Las coordenadas de origen son dentro de la imagen completa (filmstrip)
	// Y las coordenadas de destino son dentro del area del componente donde se dibuja el knob
	// frameSize se obtuvo en el constructor llamando a getWidth() de la imagen (ancho de cada frame)
	// Como la imagen es cuadrada (frameSize x frameSize), el ancho de cada frame es igual al alto de cada frame
	// Luego srcY muestra la posicion vertical del frame dentro de la filmstrip
	// srcX es 0 porque los frames estan apilados verticalmente, entonces el origen X siempre es 0
    const int srcX = 0;
    const int srcY = frameIndex * frameSize;

	// Aca centramos el knob en el area disponible
	// jmin nos da el tamaño minimo entre width y height
	// El tamaño del knob sera un cuadrado de "size x size"
	// Y luego, destX y destY son las coordenadas donde se dibuja el knob centrado
    const int size = juce::jmin(width, height);
    const int destX = x + (width - size) / 2;
    const int destY = y + (height - size) / 2;

	// Finalmente dibujamos el frame correspondiente
	// Parametros: imagen, destinoX, destinoY, destinoWidth, destinoHeight, sourceX, sourceY, sourceWidth, sourceHeight
	// destinoX y destinoY son las coordenadas donde se dibuja la imagen en el componente, al igual que destinoWidth y destinoHeight
	// sourceX y sourceY son las coordenadas dentro de la imagen de donde se toma el recorte, al igual que sourceWidth y sourceHeight
	// Finalmente, size es el tamaño del knob (width y height son el area disponible)
    g.drawImage(knobStrip, destX, destY, size, size, srcX, srcY, frameSize, frameSize);
}