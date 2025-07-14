#include <zx/zx.hpp>

#include "matchers.hpp"

TEST_CASE("char32 - split", "")
{
    REQUIRE_THAT(zx::char32::split(""), matchers::is_empty());
    REQUIRE_THAT(zx::char32::split("ABC"), matchers::elements_are('A', 'B', 'C'));
    REQUIRE_THAT(
        zx::char32::split("żółty"),
        matchers::elements_are(
            zx::char32{ "ż" }, zx::char32{ "ó" }, zx::char32{ "ł" }, zx::char32{ "t" }, zx::char32{ "y" }));
    REQUIRE_THAT(
        zx::char32::split("Θάλασσα"),
        matchers::elements_are(
            zx::char32{ "Θ" },
            zx::char32{ "ά" },
            zx::char32{ "λ" },
            zx::char32{ "α" },
            zx::char32{ "σ" },
            zx::char32{ "σ" },
            zx::char32{ "α" }));
    REQUIRE_THAT(zx::str(zx::delimit(zx::char32::split("Θάλασσα"), "")), matchers::equal_to("Θάλασσα"));
    REQUIRE_THAT(
        zx::char32::split("῎Ανδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ").take(24),
        matchers::elements_are(
            zx::char32{ "῎" },
            zx::char32{ "Α" },
            zx::char32{ "ν" },
            zx::char32{ "δ" },
            zx::char32{ "ρ" },
            zx::char32{ "α" },
            zx::char32{ " " },
            zx::char32{ "μ" },
            zx::char32{ "ο" },
            zx::char32{ "ι" },
            zx::char32{ " " },
            zx::char32{ "ἔ" },
            zx::char32{ "ν" },
            zx::char32{ "ν" },
            zx::char32{ "ε" },
            zx::char32{ "π" },
            zx::char32{ "ε" },
            zx::char32{ "," },
            zx::char32{ " " },
            zx::char32{ "Μ" },
            zx::char32{ "ο" },
            zx::char32{ "ῦ" },
            zx::char32{ "σ" },
            zx::char32{ "α" }));

    REQUIRE_THAT(
        zx::char32::split("👿💀🌍🔄"),
        matchers::elements_are(zx::char32{ "👿" }, zx::char32{ "💀" }, zx::char32{ "🌍" }, zx::char32{ "🔄" }));
}
