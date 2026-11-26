set(GA_WORKBENCH_PUBLIC_HEADERS
    include/GAWorkbenchTypes.h
    workflow/include/IGAArtifactRepository.h
    workflow/include/GAWorkflowSession.h
    workflow/include/GATaskScheduler.h
    workflow/include/GAArtifactStore.h
)
set(GA_WORKBENCH_SOURCES
    workflow/src/GAWorkflowSession.cpp
    workflow/src/GATaskScheduler.cpp
    workflow/src/GAArtifactStore.cpp
)

if(GA_BUILD_FILE_IO)
    list(APPEND GA_WORKBENCH_PUBLIC_HEADERS use_cases/io/include/GAFileUseCases.h)
    list(APPEND GA_WORKBENCH_SOURCES use_cases/io/src/GAFileUseCases.cpp)
endif()
