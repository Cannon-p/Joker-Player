#pragma once

#include <JuceHeader.h>

//==============================================================================
namespace aur
{
/** Small helper that drives smooth value transitions for UI components using the
    JUCE 9 animation module.

    Each animated value is stored in a component's properties under an Identifier.
    The value eases from its current value towards a target, repainting the
    component on every frame, so paint()/draw() code can simply read the current
    value and the transition happens automatically.

    Usage:
        // start easing the "hover" property of `button` towards 1.0
        aur::UIAnimator::animate (button, "hover", 1.0f, 120.0);
        // inside paint:
        const float h = aur::UIAnimator::value (button, "hover", 0.0f);
*/
class UIAnimator : private juce::Timer
{
public:
    /** Eases `comp`'s `prop` property towards `target` over `durationMs`. */
    static void animate (juce::Component& comp, const juce::Identifier& prop,
                         float target, double durationMs = 140.0);

    /** Returns the current animated value of a component property. */
    static float value (const juce::Component& comp, const juce::Identifier& prop,
                        float fallback);

    static UIAnimator& instance();

private:
    UIAnimator() = default;
    void timerCallback() override;

    juce::AnimatorUpdater updater;
};

} // namespace aur
