// v0.45 FEWER DOORS (included by PluginEditor.cpp): one tile = one door, behind it a few rooms (sub-pages) on a strip of chips.
// Rooms are created only when first opened (fast to open, light on memory).
class DoorPage : public Component
{
public:
    struct Room { String name, tip; std::function<std::unique_ptr<Component>()> make; std::unique_ptr<Component> page; };
    DoorPage (KKLookAndFeel& l) : lnf (l) {}
    void addRoom (const String& name, const String& tip, std::function<std::unique_ptr<Component>()> make)
    {
        rooms.push_back ({ name, tip, std::move (make), nullptr });
        auto b = std::make_unique<HotButton> (lnf, name); b->framed = true; b->setTooltip (tip);
        const int i = (int) rooms.size() - 1;
        b->onClick = [this, i] { show (i); };
        addAndMakeVisible (*b); chips.push_back (std::move (b));
        if (rooms.size() == 1) show (0);
        resized();
    }
    void show (int i)
    {
        if (! isPositiveAndBelow (i, (int) rooms.size())) return;
        auto& r = rooms[(size_t) i];
        if (! r.page) { r.page = r.make(); addChildComponent (*r.page); }
        for (int k = 0; k < (int) rooms.size(); ++k) { if (rooms[(size_t) k].page) rooms[(size_t) k].page->setVisible (k == i); chips[(size_t) k]->selected = k == i; chips[(size_t) k]->repaint(); }
        current = i; chips[(size_t) i]->setVisible (rooms.size() > 1);
        resized();
    }
    int currentRoom() const { return current; }
    Component* room (int i) { if (! isPositiveAndBelow (i, (int) rooms.size())) return nullptr; if (! rooms[(size_t) i].page) { rooms[(size_t) i].page = rooms[(size_t) i].make(); addChildComponent (*rooms[(size_t) i].page); resized(); } return rooms[(size_t) i].page.get(); }
    // the first room of a type (opened), e.g. find<AlchemyPage>()
    template <class T> T* find()
    {
        for (int i = 0; i < (int) rooms.size(); ++i)
            if (auto* t = dynamic_cast<T*> (room (i))) { show (i); return t; }
        return nullptr;
    }
    void paint (Graphics& g) override
    {
        if (rooms.size() < 2) return;
        g.setColour (Colour (0xff07080d).withAlpha (0.9f)); g.fillRoundedRectangle (strip().toFloat(), 12);
    }
    void resized() override
    {
        const bool multi = rooms.size() > 1;
        auto s = strip().reduced (6, 5);
        const int cw = jmin (150, (s.getWidth() + 6) / jmax (1, (int) chips.size()));   // every chip the same width
        for (auto& c : chips) { c->setVisible (multi); c->setBounds (s.removeFromLeft (cw).withTrimmedRight (6)); }
        for (auto& r : rooms) if (r.page) r.page->setBounds (multi ? getLocalBounds().withTrimmedTop (strip().getBottom() + 4) : getLocalBounds());
    }
    Component* shown() { return room (current); }   // the room you are looking at
private:
    Rectangle<int> strip() const { return { 0, 0, jmin (getWidth(), 160 * jmax (1, (int) rooms.size()) + 12), 44 }; }
    KKLookAndFeel& lnf;
    std::vector<Room> rooms;
    std::vector<std::unique_ptr<HotButton>> chips;
    int current = 0;
};
