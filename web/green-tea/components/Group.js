function Group({ namaGroup, lastMessage }) {
    return (
        <div className="flex items-center cursor-pointer p-2 hover:bg-gray-200">
            <img className="w-12 h-12 rounded-full" src="profile.jpeg" alt="" />
            <div className="ml-4">
                <h2>{namaGroup}</h2>
                <p className="text-gray-400">{lastMessage}</p>
            </div>
        </div>
    );
}

export default Group;
