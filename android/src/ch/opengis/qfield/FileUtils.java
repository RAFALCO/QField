package ch.opengis.qfield;

import android.content.Context;
import android.net.Uri;
import android.database.Cursor;
import android.util.Log;
import android.app.Activity;
import android.content.ContentResolver;
import android.os.Environment;
import android.provider.DocumentsContract;

class FileUtils{

    private static final String TAG = "MyCloudProvider";

    public static String getPathFromUri(Uri uri, ContentResolver resolver){

        Log.v(TAG, "Uri Scheme: " + uri.getScheme());
        Log.v(TAG, "Authority: " + uri.getAuthority());
        Log.v(TAG, "Query: " + uri.getQuery());
        Log.v(TAG, "Segments: " + uri.getPathSegments().toString());

        Log.v(TAG, "getDataDirectory: " + Environment.getDataDirectory());
        Log.v(TAG, "getExternalstoragedirectory: " + Environment.getExternalStorageDirectory());
        Log.v(TAG, "getRootdirectory: " + Environment.getRootDirectory());

        if (uri.getAuthority().equals("ch.opengis.qfield.documents")){
            Cursor cursor = null;
            try {
                cursor = resolver.query(uri, null, null, null, null);
                if (cursor.moveToFirst()) {
                    return cursor.getString(0);
                }
            } catch (Exception e) {
                // Eat it
            }

        }else if (uri.getAuthority().equals("com.android.externalstorage.documents")){
            final String docId = DocumentsContract.getDocumentId(uri);

            Log.v(TAG, "---doc Id: " + docId);
            final String[] split = docId.split(":");
            final String type = split[0];

            if ("primary".equalsIgnoreCase(type)) {
                return Environment.getExternalStorageDirectory() + "/" + split[1];
            }else{
                return "/storage/" + type + "/" + split[1];
            }
        }

        return null;
    }
}
