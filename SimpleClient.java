import java.io.IOException;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.Iterator;
import java.util.Scanner;

public class SimpleClient {
    public static void main (String[] args) {
        try {
            SocketChannel channel = SocketChannel.open(new InetSocketAddress("127.0.0.1",2379));
            channel.configureBlocking(false);
            channel.setOption(StandardSocketOptions.SO_RCVBUF,0);
            Selector selector = Selector.open();
            channel.register(selector,SelectionKey.OP_READ);
            Scanner scanner = new Scanner(System.in);
            new Thread(){
                @Override
                public void run() {
                    StringBuilder builder = new StringBuilder();
                    while (true) {
                        try {
                            selector.select();
                            Iterator<SelectionKey> iterator = selector.selectedKeys().iterator();
                            while (iterator.hasNext()){
                                SelectionKey key = iterator.next();
                                if(key.isReadable()){
                                    ByteBuffer buffer = ByteBuffer.allocate(4096);
                                    int hasRead;
                                    SocketChannel sc = (SocketChannel) key.channel();
                                    while ((hasRead = sc.read(buffer))!=0){
                                        builder.append(new String(buffer.array(),0,hasRead));
                                    }
                                    String res = builder.toString();
                                    if(res.equals("PING")){
                                        channel.write(ByteBuffer.wrap("PONG".getBytes()));
                                    }else{
                                        System.out.println(res);
                                    }
                                    builder.delete(0,builder.length());
                                }
                            }
                        }
                        catch (IOException e){

                        }
                    }
                }
            }.start();
            while (true) {
                channel.write(ByteBuffer.wrap(scanner.nextLine().getBytes()));
            }
        }
        catch (UnknownHostException e){

        }catch (IOException e){

        }
    }
}