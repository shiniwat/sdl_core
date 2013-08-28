package com.ford.syncV4.protocol;

import com.ford.syncV4.protocol.enums.FrameData;
import com.ford.syncV4.protocol.enums.FrameDataControlFrameType;
import com.ford.syncV4.protocol.enums.FrameType;
import com.ford.syncV4.protocol.enums.SessionType;

public class ProtocolFrameHeaderFactory {


    public static ProtocolFrameHeader createMobileNavStartSession(int messageID, byte sessionID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Control);
        msg.setSessionType(SessionType.Mobile_Nav);
        msg.setFrameData(FrameDataControlFrameType.StartSession.value());
        msg.setMessageID(messageID);
        msg.setSessionID(sessionID);
        msg.setCompressed(false);
        msg.setDataSize(0);
        return msg;
    }

    public static ProtocolFrameHeader createStartSession(SessionType serviceType, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Control);
        msg.setSessionType(serviceType);
        msg.setFrameData(FrameDataControlFrameType.StartSession.value());
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createStartSessionACK(SessionType serviceType, byte sessionID, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Control);
        msg.setSessionType(serviceType);
        msg.setSessionID(sessionID);
        msg.setFrameData(FrameDataControlFrameType.StartSessionACK.value());
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createStartSessionNACK(SessionType serviceType, byte sessionID, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Control);
        msg.setSessionType(serviceType);
        msg.setSessionID(sessionID);
        msg.setFrameData(FrameDataControlFrameType.StartSessionNACK.value());
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createEndSession(SessionType serviceType,
                                                       byte sessionID, int messageID,
                                                       byte version,
                                                       int dataLength) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Control);
        msg.setSessionType(serviceType);
        msg.setSessionID(sessionID);
        msg.setFrameData(FrameDataControlFrameType.EndSession.value());
        msg.setMessageID(messageID);
        msg.setDataSize(dataLength);

        return msg;
    }

    public static ProtocolFrameHeader createSingleSendData(SessionType serviceType, byte sessionID,
                                                           int dataLength, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Single);
        msg.setSessionType(serviceType);
        msg.setFrameData(ProtocolFrameHeader.FrameDataSingleFrame);
        msg.setSessionID(sessionID);
        msg.setDataSize(dataLength);
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createMultiSendDataFirst(SessionType serviceType, byte sessionID,
                                                               int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.First);
        msg.setSessionType(serviceType);
        msg.setFrameData(ProtocolFrameHeader.FrameDataFirstFrame);
        msg.setSessionID(sessionID);
        msg.setDataSize(8);
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createMultiSendDataRest(SessionType serviceType, byte sessionID,
                                                              int dataLength, byte frameSequenceNumber, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Consecutive);
        msg.setSessionType(serviceType);
        msg.setFrameData(frameSequenceNumber/*FrameData.ConsecutiveFrame.value()*/);
        msg.setSessionID(sessionID);
        msg.setDataSize(dataLength);
        msg.setMessageID(messageID);

        return msg;
    }

    public static ProtocolFrameHeader createMultiSendDataRest(SessionType serviceType, byte sessionID,
                                                              int dataLength, int messageID, byte version) {
        ProtocolFrameHeader msg = new ProtocolFrameHeader();
        msg.setVersion(version);
        msg.setFrameType(FrameType.Consecutive);
        msg.setSessionType(serviceType);
        msg.setFrameData(FrameData.ConsecutiveFrame.value());
        msg.setSessionID(sessionID);
        msg.setDataSize(dataLength);
        msg.setMessageID(messageID);

        return msg;
    }

    public static BinaryFrameHeader createBinaryFrameHeader(byte rpcType, int functionID, int corrID, int jsonSize) {
        BinaryFrameHeader msg = new BinaryFrameHeader();
        msg.setRPCType(rpcType);
        msg.setFunctionID(functionID);
        msg.setCorrID(corrID);
        msg.setJsonSize(jsonSize);

        return msg;
    }

}
