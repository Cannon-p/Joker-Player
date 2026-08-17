#include "UIAnimator.h"

namespace aur
{
//==============================================================================
void UIAnimator::animate (juce::Component& comp, const juce::Identifier& prop,
                          float target, double durationMs)
{
    // Store the requested target so repaint-driven calls don't spawn a fresh
    // animator on every frame while one is already heading for the same value.
    const auto targetKey = juce::Identifier (prop.toString() + "Target");
    constexpr float sentinel = -1.0e9f;
    const auto existingTarget = (float) comp.getProperties().getWithDefault (targetKey, (float) sentinel);

    if (juce::approximatelyEqual (existingTarget, target))
        return;

    const float start = comp.getProperties().getWithDefault (prop, target);

    auto safe = juce::Component::SafePointer<juce::Component> (&comp);

    auto animator = juce::ValueAnimatorBuilder{}
                        .withDurationMs (durationMs)
                        .withEasing (juce::Easings::createEaseInOut())
                        .withValueChangedCallback ([safe, prop, start, target] (float t)
                                                   {
                                                       if (safe.getComponent() == nullptr)
                                                           return;

                                                       safe->getProperties().set (prop,
                                                                                  start + (target - start) * t);
                                                       safe->repaint();
                                                   })
                        .withOnCompleteCallback ([safe, targetKey, target] ()
                                                 {
                                                     if (safe.getComponent() == nullptr)
                                                         return;

                                                     // Remember the reached target so paint-time
                                                     // calls with the same value don't re-animate,
                                                     // while a different target still transitions.
                                                     safe->getProperties().set (targetKey, target);
                                                 })
                        .build();

    instance().updater.addAnimator (animator);
    animator.start();

    // The timer is only really needed while animations are running; keep it
    // cheap so the UI never falls back to a static look mid-transition.
    instance().startTimerHz (120);
}

float UIAnimator::value (const juce::Component& comp, const juce::Identifier& prop,
                         float fallback)
{
    return comp.getProperties().getWithDefault (prop, fallback);
}

UIAnimator& UIAnimator::instance()
{
    static UIAnimator instance;
    return instance;
}

void UIAnimator::timerCallback()
{
    updater.update();
}

} // namespace aur
