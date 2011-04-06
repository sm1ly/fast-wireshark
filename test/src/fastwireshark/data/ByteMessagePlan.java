package fastwireshark.data;

public class ByteMessagePlan extends Message{

	private final byte[] bytes;
	
	/**
	 * Creates a new byte message plan with the given bytes
	 * @param bytes The bytes to use in the message
	 */
	public ByteMessagePlan(byte[] bytes, String from, String to){
		super(from,to);
		if(bytes == null){
			throw new IllegalArgumentException("bytes is null");
		}
		this.bytes = bytes;
	}
	
	public byte[] getBytes(){
		byte[] t = new byte[bytes.length];
		System.arraycopy(bytes, 0, t, 0, bytes.length);
		return t;
	}
}
