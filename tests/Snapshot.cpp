// LuckyNotesSnapshot <out.png> [scale]: renders the plugin editor headless (software renderer) into a PNG
// for the shop page. Uses a fixed state (F minor, 150 BPM, seed 7, row 03 previewing) so the shot is reproducible.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String outPath = argc > 1 ? juce::String (argv[1]) : juce::String ("LuckyNotes-UI.png");
    const double scale = argc > 2 ? juce::jlimit (0.8, 1.5, juce::String (argv[2]).getDoubleValue()) : 1.5;

    int code = 0;
    {
        auto proc = std::make_unique<LuckyNotesProcessor>();

        juce::ValueTree t ("LUCKYNOTES");
        t.setProperty ("root", 5, nullptr);
        t.setProperty ("minor", true, nullptr);
        t.setProperty ("lucky", false, nullptr);
        t.setProperty ("bars", 8, nullptr);
        t.setProperty ("sync", true, nullptr);
        t.setProperty ("bpm", 150.0, nullptr);
        t.setProperty ("sound", 1, nullptr);
        t.setProperty ("seed", "7", nullptr);
        t.setProperty ("keyRoot", 5, nullptr);
        t.setProperty ("keyMinor", true, nullptr);
        t.setProperty ("ui", scale, nullptr);
        juce::MemoryBlock state;
        juce::AudioProcessor::copyXmlToBinary (*t.createXml(), state);
        proc->setStateInformation (state.getData(), (int) state.getSize());

        // run the preview for a moment so the playhead sits inside the loop
        proc->setRateAndBufferSizeDetails (48000.0, 512);
        proc->prepareToPlay (48000.0, 512);
        proc->startPreview (2);
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        int midiEvents = 0;
        for (int i = 0; i < (int) (48000 * 4.3 / 512); ++i)
        {
            proc->processBlock (buf, midi);
            midiEvents += midi.getNumEvents();
        }
        std::printf ("preview: %d MIDI events, beat %.2f, peak %.3f\n", midiEvents, proc->getPreviewBeat(), buf.getMagnitude (0, 512));

        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
        ed->setSize (juce::roundToInt (LuckyNotesEditor::kBaseWidth * scale), juce::roundToInt (LuckyNotesEditor::kBaseHeight * scale));
        ed->setVisible (true);
        if (auto* le = dynamic_cast<LuckyNotesEditor*> (ed.get())) le->refreshNow();

        juce::Image img (juce::Image::ARGB, ed->getWidth(), ed->getHeight(), true, juce::SoftwareImageType());
        {
            juce::Graphics g (img);
            ed->paintEntireComponent (g, true);
        }

        // sanity: the picture must not be blank
        std::set<juce::uint32> colours;
        for (int y = 0; y < img.getHeight(); y += 7)
            for (int x = 0; x < img.getWidth(); x += 7)
                colours.insert (img.getPixelAt (x, y).getARGB());
        std::printf ("snapshot %dx%d, %d distinct sampled colours\n", img.getWidth(), img.getHeight(), (int) colours.size());
        if (colours.size() < 20) { std::printf ("ERROR: snapshot looks blank\n"); code = 1; }

        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (outPath));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat png;
        if (! os.openedOk() || ! png.writeImageToStream (img, os)) { std::printf ("ERROR: cannot write %s\n", out.getFullPathName().toRawUTF8()); code = 1; }
        else std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());

        ed = nullptr;
        proc->releaseResources();
        proc = nullptr;
    }
    return code;
}
