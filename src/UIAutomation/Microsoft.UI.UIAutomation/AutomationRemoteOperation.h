// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Microsoft.UI.UIAutomation.AutomationRemoteOperation.g.h"
#include "AutomationRemoteOperationResultSet.h"

#include <winrt/Windows.UI.UIAutomation.h>
#include <winrt/Windows.UI.UIAutomation.Core.h>

#include "RemoteOperationInstructions.h"
#include "RemoteOperationGraph.h"

#include <memory>
#include <utility>


namespace winrt::Microsoft::UI::UIAutomation::implementation
{
    // This class represents the main entry point to Remote Operations.
    //
    // There are two primary ways to add new instructions to be executed as part of the remote operation:
    //
    //   1. Import an existing IUIAutomation{Element,TextRange,etc.} object, obtaining a local "stand-in"
    //      for a remote object. Calling methods on the stand-in adds new instructions to the operation.
    //      Most methods on the stand-in objects have a return value that represents a stand-in for the
    //      object returned by that operation. This can be used to continue chaining operations.
    //
    //   2. Create a new remote object as part of the operation. This is done with methods like NewInt or
    //      NewBool. These methods also return a stand-in, that exposes operations that can be triggered on
    //      the object as part of the remote operation. The difference is that these objects alone aren't
    //      tied to any particular provider process.
    //
    // Note that if the client imports objects that belong to different connections/processes, the operation
    // will return failure.
    struct AutomationRemoteOperation : AutomationRemoteOperationT<AutomationRemoteOperation>
    {
        AutomationRemoteOperation();

        // Internal

        // Inserts a new instruction into the operation's current scope.
        // Ultimately, all "stand-in" objects eventually need to make a call to this
        // method to insert any useful work that should occur on the provider side.
        void InsertInstruction(const bytecode::Instruction& instruction);
        // Returns a brand new ID that can be used to identify operands in the remote
        // operation (e.g. to assign the result of an instruction to an operand).
        bytecode::OperandId GetNextId();
        // Requests that the operand with the given ID be marshaled back to the client
        // as part of the response.
        void RequestResponse(bytecode::OperandId remoteOperationId);

        // API
        winrt::AutomationRemoteBool NewBool(bool initialValue);
        winrt::AutomationRemoteInt NewInt(int32_t initialValue);
        winrt::AutomationRemoteUint NewUint(uint32_t initialValue);
        winrt::AutomationRemoteDouble NewDouble(double initialValue);
        winrt::AutomationRemoteChar NewChar(wchar_t initialValue);
        winrt::AutomationRemoteString NewString(hstring const& initialValue);
        winrt::AutomationRemotePoint NewPoint(Windows::Foundation::Point const& initialValue);
        winrt::AutomationRemoteRect NewRect(Windows::Foundation::Rect const& initialValue);
        bool IsGuidSupported() const;
        winrt::AutomationRemoteGuid NewGuid(const winrt::guid& initialValue);
        bool IsCacheRequestSupported() const;
        winrt::AutomationRemoteCacheRequest NewCacheRequest();
        winrt::AutomationRemoteArray NewArray();
        winrt::AutomationRemoteStringMap NewStringMap();
        winrt::AutomationRemoteAnyObject NewNull();
        winrt::AutomationRemoteAnyObject NewEmpty();
        winrt::AutomationRemoteByteArray NewByteArray(winrt::array_view<const uint8_t> initialValue);

        // Returns whether the given opcode is supported in the current remote
        // operation connection. Calls directly into the corresponding
        // CoreAutomationRemoteOperation API. Throws E_FAIL if no connection is
        // currently active.
        // TODO #77: Specify which Windows release we start throwing that error.
        bool IsOpcodeSupported(uint32_t opcode) const;

        winrt::AutomationRemoteElement ImportElement(winrt::Windows::UI::UIAutomation::AutomationElement const& element);
        winrt::AutomationRemoteTextRange ImportTextRange(winrt::Windows::UI::UIAutomation::AutomationTextRange const& textRange);
        winrt::AutomationRemoteConnectionBoundObject ImportConnectionBoundObject(winrt::Windows::UI::UIAutomation::AutomationConnectionBoundObject const& connectionBoundObject);

        AutomationRemoteOperationResponseToken RequestResponse(const winrt::AutomationRemoteObject& object);

        void IfBlock(
            const winrt::AutomationRemoteBool& condition,
            const AutomationRemoteOperationScopeHandler& trueHandler,
            const AutomationRemoteOperationScopeHandler& falseHandler = nullptr);
        void WhileBlock(
            const winrt::AutomationRemoteBool& condition,
            const AutomationRemoteOperationScopeHandler& loopBodyHandler,
            const AutomationRemoteOperationScopeHandler& loopConditionUpdateHandler = nullptr);

        void ReturnOperationStatus(winrt::hresult status);
        void ReturnOperationStatus(const winrt::AutomationRemoteInt& status);

        void BreakLoop();
        void ContinueLoop();

        void TryBlock(
            const AutomationRemoteOperationScopeHandler& tryBodyHandler);

        void TryBlock(
            const AutomationRemoteOperationScopeHandler& tryBodyHandler,
            const AutomationRemoteOperationScopeHandler& exceptBlockHandler);

        winrt::AutomationRemoteInt GetCurrentFailureCode();

        winrt::AutomationRemoteOperationResultSet Execute();

#include "AutomationRemoteOperationMethods.g.h"

    private:

        // Members

        // The ID is incremented every time a new remote OperandId is requested. The remote operation
        // is the only "source of truth" for operand IDs; this way we get a unique ID for any operand
        // in the operation.
        int m_nextId = 1;

        // The root operation graph; operation "scopes" (such as if blocks) are going to be a subgraph of
        // the root graph.
        std::shared_ptr<RemoteOperationGraph> m_rootGraph = std::make_shared<RemoteOperationGraph>();

        // The scope that any newly added instructions are added into. This allows us to build up
        // different scopes independently, but using the same API, by simply changing which
        // scope is considered "current".
        std::shared_ptr<RemoteOperationGraph> m_currentScope;

        // The underlying platform Remote Operation that we're preparing for execution.
        winrt::Windows::UI::UIAutomation::Core::CoreAutomationRemoteOperation m_remoteOperation;
    };
}
namespace winrt::Microsoft::UI::UIAutomation::factory_implementation
{
    struct AutomationRemoteOperation : AutomationRemoteOperationT<AutomationRemoteOperation, implementation::AutomationRemoteOperation>
    {
    };
}
