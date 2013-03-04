package lry.FFmpegLry;

import java.io.File;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;

public class FFmpegLryActivity extends Activity {
    /** Called when the activity is first created. */
	
	ImageView imageView = null;
	Button btnFree = null;
	Button btnFrame = null;
	
	//ȫ�ֱ���
	private Bitmap mBitmap;
	private int mSecs = 0;
	
	//���붯̬��
    static {
        System.loadLibrary("ffmpeg");
        }
    public native String GetFFmpegVersion();
    
    //������̬�⺯��
    private static native void EnvConfigure();
	private static native void drawFrame(Bitmap bitmap);
	private static native void EnvFree();
    
    
    @Override    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        imageView = (ImageView)findViewById(R.id.frame);
        btnFrame = (Button)findViewById(R.id.frame_adv);
        btnFree = (Button)findViewById(R.id.btnFree);
        
        //�������
        System.out.println("Hello, ffmpeg~");
        
        //��ȡ�汾���кţ�����ӡ
        System.out.println(GetFFmpegVersion()); 
        
        //����bitmap����
        mBitmap = Bitmap.createBitmap(320, 240, Bitmap.Config.ARGB_8888);
        
        //���뻷������׼��
        String filePath = Environment.getExternalStorageDirectory() + File.separator;
        filePath = filePath + "external_sdcard/video/352x288.264";  //ƽ��Ŀ¼
//        filePath = filePath + "video/352x288.264";
        System.out.println("path: " + filePath);
        EnvConfigure();
        
        //���ð�ťˢ��֡
        btnFrame.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				//������ʾͼ��
				drawFrame(mBitmap);				
				imageView.setImageBitmap(mBitmap);
			}
		});
        
        //���ð�ť�ͷ��ڴ�
        btnFree.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
//				EnvFree();
				btnFrame.setEnabled(false);
				btnFree.setEnabled(false);
			}
		});
        
    } 
    
    
    
}