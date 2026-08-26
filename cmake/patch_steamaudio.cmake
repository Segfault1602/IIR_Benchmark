# Removes a target() attribute that upstream applies to a pointer-to-member-function *data member*
# in core/src/core/iir.h:
#
#     float8_t (IPL_FLOAT8_ATTR IIRFilterer::* m_applyFloat8)(float8_t);
#
# GCC ignores it, but Clang rejects it outright ("'target' attribute only applies to functions"),
# which breaks every translation unit that includes iir.h once IPL_ENABLE_FLOAT8 is on. The
# attribute is meaningless on the pointer type anyway - the functions it points at carry their own -
# and the member belongs to the IIRFilterer wrapper, which this benchmark does not use.
#
# Re-running this is a no-op, so it is safe as a FetchContent PATCH_COMMAND.

set(IIR_HEADER "${STEAMAUDIO_SOURCE_DIR}/core/src/core/iir.h")

if(NOT EXISTS "${IIR_HEADER}")
    message(FATAL_ERROR "Cannot patch SteamAudio: '${IIR_HEADER}' not found")
endif()

file(READ "${IIR_HEADER}" contents)

set(ATTRIBUTED "float8_t (IPL_FLOAT8_ATTR IIRFilterer::* m_applyFloat8)(float8_t);")
set(FIXED "float8_t (IIRFilterer::* m_applyFloat8)(float8_t);")

if(contents MATCHES "IPL_FLOAT8_ATTR IIRFilterer::\\* m_applyFloat8")
    string(REPLACE "${ATTRIBUTED}" "${FIXED}" contents "${contents}")
    file(WRITE "${IIR_HEADER}" "${contents}")
    message(STATUS "Patched SteamAudio iir.h for Clang compatibility")
elseif(contents MATCHES "m_applyFloat8")
    # Already patched, or upstream fixed it. Either way there is nothing to do.
else()
    # The member is gone entirely, so this script is aimed at a header it no longer understands.
    # Say so rather than silently doing nothing, because the symptom otherwise shows up much later
    # as an opaque Clang error inside a dependency.
    message(WARNING "SteamAudio patch found no 'm_applyFloat8' member in iir.h; upstream layout "
                    "has changed, so cmake/patch_steamaudio.cmake may no longer be needed or may "
                    "need updating.")
endif()
