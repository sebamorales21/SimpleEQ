/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// CustomRotarySlider es una subclase de Slider que ya tiene el estilo rotary y sin caja de texto.
// Asi evitamos repetir ese codigo cada vez que creamos un slider de este tipo.
// CustomRotarySlider es una clase que en su cuerpo solo tiene un constructor.
// El constructor llama al constructor de la clase base Slider con los parametros adecuados.    
// El constructor de una clase derivada puede llamar al constructor de la clase base usando la sintaxis de lista de inicializacion. Como en este caso.
struct CustomRotarySlider : juce::Slider 
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) 
    {

    }
};

struct MyLookAndFeel : juce::LookAndFeel_V4
{
    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Tamaño del círculo (el menor lado)
        const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());

        // Rectángulo centrado
        auto circleBounds = juce::Rectangle<float>()
            .withSizeKeepingCentre(size * 0.6f, size * 0.6f)
            .withCentre(bounds.getCentre());

        // Color según estado
        g.setColour(button.getToggleState() ? juce::Colours::grey
            : juce::Colours::green);

        g.fillEllipse(circleBounds);
    }

    void drawRotarySlider(juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Cuerpo del knob
        g.setColour(juce::Colours::rebeccapurple);
        g.fillEllipse(rx, ry, rw, rw);

        // Indicador del knob
        juce::Path p;
        p.addRectangle(-2.0f, -radius, 4.0f, radius * 0.6f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::antiquewhite);
        g.fillPath(p);
	}
};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf) : leftChannelFifo(&scsf)
    {
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
	}

    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }

private:
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChannelFifo;

    juce::AudioBuffer<float> monoBuffer;

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathProducer;

    juce::Path leftChannelFFTPath;
};

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent() override;
    void paint(juce::Graphics& g) override;
	void resized() override;

	// Funciones provenientes de AudioProcessorParameter::Listener
    void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

	// Funcion proveniente de Timer
	void timerCallback() override;

private:
    SimpleEQAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;

    void updateChain();

    juce::Image background;
	juce::Rectangle<int> getRenderArea();
	juce::Rectangle<int> getAnalysisArea(); // Area donde las lineas de gain se dibujan, mas chicas que el area total del componente, para que no choque con los limites

	PathProducer leftPathProducer, rightPathProducer;

};

//==============================================================================

class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    MyLookAndFeel lnf;

	CustomRotarySlider peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, highCutSlopeSlider;

	ResponseCurveComponent responseCurveComponent;
    
	// Estos Attachement son como los cables que conectan los sliders con los parametros del APVTS.
	// Deben ser inicializados en el constructor del Editor, en la lista de inicializacion.
	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;
	Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;

	juce::ToggleButton lowCutBypassButton, highCutBypassButton, peakBypassButton, analyzerBypassButton;

	using ButtonAttachment = APVTS::ButtonAttachment;
	ButtonAttachment lowCutBypassButtonAttachment, highCutBypassButtonAttachment, peakBypassButtonAttachment, analyzerBypassButtonAttachment;

	std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
