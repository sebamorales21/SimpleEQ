/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
	const auto& params = audioProcessor.getParameters();
	for (auto* param : params) {
		param->addListener(this);
	}

	// Construye la cadena de filtros inicial antes de que se pinte por primera vez
	updateChain();
		
	// Inicia el timer para que llame a timerCallback cada 50ms
	startTimer(50);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
	const auto& params = audioProcessor.getParameters();
	for (auto* param : params) {
		param->removeListener(this);
	}
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
	using namespace juce;

	g.drawImage(background, getLocalBounds().toFloat());

	auto responseArea = getAnalysisArea();
	auto w = responseArea.getWidth();

	auto& lowcut = monoChain.get<ChainPositions::LowCut>();
	auto& peak = monoChain.get<ChainPositions::Peak>();
	auto& highcut = monoChain.get<ChainPositions::HighCut>();
		
	auto sampleRate = audioProcessor.getSampleRate();

	// En este vector se guardaran las magnitudes de cada frecuencia
	// Necesitamos una magnitud por cada pixel de ancho en la zona de respuesta
	std::vector<double> mags;
	mags.resize(w);

	// Ahora, iteramos por cada pixel de ancho, y calculamos la magnitud para cada frecuencia
	for (int i = 0; i < w; ++i) {
		// Convertimos la posicion x (i) en una frecuencia logaritmica entre 20Hz y 20kHz
		auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

		// Empezamos con una magnitud de 1.0 (0 dB)
		double mag = 1.0;

		if (!monoChain.isBypassed<ChainPositions::Peak>())
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);  // Aca uso -> porque peak es un IIRFilter y coefficients es un puntero a IIRCoefficients dentro de IIRFilter

		if (!lowcut.isBypassed<0>())
			mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<1>())
			mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<2>())
			mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowcut.isBypassed<3>())
			mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!highcut.isBypassed<0>())
			mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<1>())
			mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<2>())
			mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highcut.isBypassed<3>())
			mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		// Guardamos la magnitud en el vector
		mags[i] = Decibels::gainToDecibels(mag);
	}

	// Ahora dibujamos la respuesta en frecuencia
	// Path es una clase de JUCE que permite dibujar lineas complejas
	Path responseCurve;

	// Funcion lambda para mapear la magnitud (en dB) a la posicion Y en la zona de respuesta, será usada dentro del bucle de dibujo
	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();
	auto map = [outputMin, outputMax](double input)
		{
			return jmap(input, -24.0, 24.0, outputMin, outputMax);
		};

	// Inicio del path en el primer punto
	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	// Inicio del area de renderizado
	g.saveState();
	g.reduceClipRegion(getRenderArea());

	// Linea a cada punto siguiente
	for (size_t i = 1; i < mags.size(); ++i) {
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
	}

	// FFT Left Channel
	auto leftChannelFFTPath = leftPathProducer.getPath();
	leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
	g.setColour(Colours::rebeccapurple);
	g.strokePath(leftChannelFFTPath, PathStrokeType(1));

	// FFT Right Channel
	auto rightChannelFFTPath = rightPathProducer.getPath();
	rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
	g.setColour(Colours::skyblue);
	g.strokePath(rightChannelFFTPath, PathStrokeType(1));

	// Dibujar la curva de respuesta
	g.setColour(Colours::white);
	g.strokePath(responseCurve, PathStrokeType(2.0f));

	// Fin del area de renderizado
	g.restoreState();

	// Dibujar el borde del area de renderizado
	g.setColour(Colours::rebeccapurple);
	g.drawRoundedRectangle(getRenderArea().toFloat(), 4.0f, 3.0f);
}

void ResponseCurveComponent::resized()
{
	using namespace juce;
	background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

	Graphics g(background);

	Array<float> freqsLines
	{ 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f,
	  100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f,
	  900.0f, 1000.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f,
	  7000.0f, 8000.0f, 9000.0f, 10000.0f, 20000.0f };

	// Obtener el area de renderizado
	auto renderArea = getAnalysisArea();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getBottom();
	auto width = renderArea.getWidth();

	// Calcular las posiciones X de las lineas de frecuencia
	Array<float> xs;
	for(auto f : freqsLines)
	{
		auto x = mapFromLog10(f, 20.0f, 20000.0f);
		xs.add(left + x * width);
	}

	// Dibujar las lineas verticales de frecuencia
	g.setColour(Colours::dimgrey);
	for(auto x : xs)
	{
		g.drawVerticalLine(static_cast<int>(x), static_cast<float>(top), static_cast<float>(bottom));
	}

	// Dibujar las lineas horizontales de ganancia
	Array<float> gainLines
	{ -24.0f, -12.0f, 0.0f, 12.0f, 24.0f };

	for (auto gDb : gainLines)
	{
		auto y = jmap(gDb, -24.0f, 24.0f, static_cast<float>(bottom), static_cast<float>(top));
		g.setColour(gDb == 0.0f ? Colours::green : Colours::dimgrey);
		g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(left), static_cast<float>(right));
	}

	// Etiquetas de frecuencia
	g.setColour(juce::Colours::lightgrey);
	constexpr int fontHeight = 11;
	g.setFont(static_cast<float>(fontHeight));

	// Solo estas etiquetas:
	constexpr std::array<float, 3> labelFreqs{ 100.0f, 1000.0f, 10000.0f };

	// Posición Y: arriba del área de análisis, con un pequeño margen
	const int textY = top - fontHeight - 6;

	// Dibujar cada etiqueta
	for (const auto f : labelFreqs)
	{
		const auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
		const auto x = static_cast<float>(left) + normX * static_cast<float>(width);
		const int  xi = juce::roundToInt(x);

		const juce::String str = (f >= 1000.0f)
			? (juce::String(juce::roundToInt(f / 1000.0f)) + " kHz")
			: (juce::String(juce::roundToInt(f)) + " Hz");

		const int textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r(xi - textWidth / 2, textY, textWidth, fontHeight);
		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	// Etiquetas de ganancia
	constexpr int rightPadding = 4; // Padding desde el borde derecho

	// Ancho fijo para las etiquetas de ganancia
	const int labelWidth = juce::jmax(g.getCurrentFont().getStringWidth("-24"), g.getCurrentFont().getStringWidth("+24")) + 2; // +2 por seguridad

	// Caja fija pegada a la derecha, pero con padding
	const int labelX = getWidth() - rightPadding - labelWidth;

	// Dibujar cada etiqueta de ganancia
	const int leftLabelWidth = juce::jmax(g.getCurrentFont().getStringWidth("-48"), g.getCurrentFont().getStringWidth("0")) + 2;

	for (const auto gDb : gainLines)
	{
		// Etiquetas de ganancia
		const auto y = juce::jmap(gDb, -24.0f, 24.0f, static_cast<float>(bottom), static_cast<float>(top));

		juce::String str;
		if (gDb > 0.0f) str << "+";
		str << gDb;

		juce::Rectangle<int> r(labelX, juce::roundToInt(y) - fontHeight / 2, labelWidth, fontHeight);

		g.setColour(gDb == 0.0f ? juce::Colours::green : juce::Colours::lightgrey);
		g.drawFittedText(str, r, juce::Justification::centred, 1);

		// Etiquetas de nivel
		str.clear();
		str << (gDb - 24.0f);

		int textWidth;
		textWidth = g.getCurrentFont().getStringWidth(str);

		r.setX(1);
		r.setWidth(leftLabelWidth);
		r.setCentre(r.getCentreX(), juce::roundToInt(y));

		g.setColour(juce::Colours::lightgrey);
		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}


}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
	auto bounds = getLocalBounds();
	bounds.removeFromTop(20);
	bounds.removeFromBottom(2);
	bounds.removeFromLeft(25);
	bounds.removeFromRight(25);

	return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
	auto renderArea = getRenderArea();
	renderArea.removeFromTop(4);
	renderArea.removeFromBottom(4);
	return renderArea;
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
	// Logica:
	// Mientras hayan buffers que pullear desde SingleChannelSimpleFifo se mandan al FFT Data Generator.
	juce::AudioBuffer<float> tempIncomingBuffer;

	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), tempIncomingBuffer.getReadPointer(0, 0), size);

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -60.0f);
		}
	}

	// Generar Path a partir de los FFT Data
	// Esta funcion va a generar un Path en los bounds que le pasemos (Segundo argumento de la funcion generatePath())
	const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
	const auto binWidth = sampleRate / (double)fftSize;

	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftData;
		if (leftChannelFFTDataGenerator.getFFTData(fftData))
		{
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.0f);
		}
	}

	// Mientras hayan Paths que se puedan pullear, pullea tantos como podamosy mostramos el mas reciente
	while (pathProducer.getNumPathsAvailable())
	{
		pathProducer.getPath(leftChannelFFTPath);
	}
}

void ResponseCurveComponent::timerCallback()
{
	auto fftBounds = getAnalysisArea().toFloat();
	fftBounds.removeFromRight(9.0f);
	auto sampleRate = audioProcessor.getSampleRate();

	leftPathProducer.process(fftBounds, sampleRate);
	rightPathProducer.process(fftBounds, sampleRate);

	if (parametersChanged.compareAndSetBool(false, true))
	{
		// Si los parametros han cambiado (true), actualizamos la cadena y repintamos y parametrosChanged a false
		updateChain();
		// repaint(); Ahora hay que repintar todo el rato (FFT Analyzer), no solo cuando cambien parametros.
	}

	repaint();
}

void ResponseCurveComponent::updateChain() {
	auto chainSettings = getChainSettings(audioProcessor.apvts);

	// Actualizar los coeficientes del filtro peak
	auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
	updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

	// Actualizar los coeficientes de los filtros low cut y high cut
	auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
	auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

	updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);

}

//==============================================================================

SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
	audioProcessor (p),
	responseCurveComponent(audioProcessor),
	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, "Peak Q", peakQualitySlider),
	lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
	highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

	// el argumento se lee como "para cada comp dentro de getComps()"
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
	}

    setSize (375, 525);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
}

void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
	using namespace juce;

    g.fillAll (Colours::black);

}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	// Esto devuelve un juce::Rectangle<int> que representa el espacio disponible. Es decir el espacio definido por setSize (375, 525);
    auto bounds = getLocalBounds();     

	// Area destinada a la respuesta y el espectrorgama
	auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

	responseCurveComponent.setBounds(responseArea);

	// Area destintada para cada seccion del EQ
	auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	// Posicionamiento de los knobs de cada seccion
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);

	highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQualitySlider.setBounds(bounds);
}

//==============================================================================

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps() {
	// Devuelve un vector con punteros a todos los sliders del editor, para luego poder iterar sobre ellos y añadirlos todos de una vez.
    return { &peakFreqSlider, &peakGainSlider, &peakQualitySlider, &lowCutFreqSlider, &highCutFreqSlider, &lowCutSlopeSlider, &highCutSlopeSlider, &responseCurveComponent };
}
